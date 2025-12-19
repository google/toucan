// Copyright 2023 The Toucan Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "codegen_llvm.h"

#include <iostream>
#include <span>

#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#if TARGET_OS_IS_WASM
#include <tint/tint.h>
#endif

#include <ast/constant_folder.h>
#include "codegen_spirv.h"

namespace Toucan {

namespace {

constexpr int kMinAutoConstantSize = 1024;

struct Intrinsic {
    const char*         methodName;
    llvm::Intrinsic::ID id;
};

constexpr Intrinsic floatIntrinsics[] = {
  "sqrt",  llvm::Intrinsic::sqrt,
  "sin",   llvm::Intrinsic::sin,
  "cos",   llvm::Intrinsic::cos,
  "tan",   llvm::Intrinsic::tan,
  "fabs",  llvm::Intrinsic::fabs,
  "floor", llvm::Intrinsic::floor,
  "ceil",  llvm::Intrinsic::ceil,
  "min",   llvm::Intrinsic::minimum,
  "max",   llvm::Intrinsic::maximum,
  "pow",   llvm::Intrinsic::pow,
};

constexpr Intrinsic boolIntrinsics[] = {
  "any", llvm::Intrinsic::vector_reduce_or,
  "all", llvm::Intrinsic::vector_reduce_and,
};

constexpr Intrinsic uintIntrinsics[] = {
  "clz", llvm::Intrinsic::ctlz,
  "min", llvm::Intrinsic::umin,
  "max", llvm::Intrinsic::umax,
};

constexpr Intrinsic intIntrinsics[] = {
  "clz", llvm::Intrinsic::ctlz,
  "min", llvm::Intrinsic::smin,
  "max", llvm::Intrinsic::smax,
};

llvm::Value* GenerateBinOpInt(LLVMBuilder*   builder,
                              BinOpNode::Op  op,
                              llvm::Value*   lhs,
                              llvm::Value*   rhs) {
  switch (op) {
    case BinOpNode::ADD: return builder->CreateAdd(lhs, rhs, "iadd");
    case BinOpNode::SUB: return builder->CreateSub(lhs, rhs, "isub");
    case BinOpNode::MUL: return builder->CreateMul(lhs, rhs, "imul");
    case BinOpNode::DIV: return builder->CreateSDiv(lhs, rhs, "idiv");
    case BinOpNode::MOD: return builder->CreateSRem(lhs, rhs, "imod");
    case BinOpNode::LT: return builder->CreateICmpSLT(lhs, rhs, "icmp");
    case BinOpNode::LE: return builder->CreateICmpSLE(lhs, rhs, "icmp");
    case BinOpNode::EQ: return builder->CreateICmpEQ(lhs, rhs, "icmp");
    case BinOpNode::GE: return builder->CreateICmpSGE(lhs, rhs, "icmp");
    case BinOpNode::GT: return builder->CreateICmpSGT(lhs, rhs, "icmp");
    case BinOpNode::NE: return builder->CreateICmpNE(lhs, rhs, "icmp");
    case BinOpNode::LOGICAL_AND:
    case BinOpNode::BITWISE_AND: return builder->CreateAnd(lhs, rhs, "and");
    case BinOpNode::LOGICAL_OR:
    case BinOpNode::BITWISE_OR: return builder->CreateOr(lhs, rhs, "or");
    case BinOpNode::BITWISE_XOR: return builder->CreateXor(lhs, rhs, "xor");
    default: assert(false); return 0;
  }
}

llvm::Value* GenerateBinOpUInt(LLVMBuilder*   builder,
                               BinOpNode::Op  op,
                               llvm::Value*   lhs,
                               llvm::Value*   rhs) {
  switch (op) {
    case BinOpNode::ADD: return builder->CreateAdd(lhs, rhs, "uiadd");
    case BinOpNode::SUB: return builder->CreateSub(lhs, rhs, "uisub");
    case BinOpNode::MUL: return builder->CreateMul(lhs, rhs, "uimul");
    case BinOpNode::DIV: return builder->CreateUDiv(lhs, rhs, "uidiv");
    case BinOpNode::MOD: return builder->CreateURem(lhs, rhs, "uimod");
    case BinOpNode::LT: return builder->CreateICmpULT(lhs, rhs, "uicmp");
    case BinOpNode::LE: return builder->CreateICmpULE(lhs, rhs, "uicmp");
    case BinOpNode::EQ: return builder->CreateICmpEQ(lhs, rhs, "uicmp");
    case BinOpNode::GE: return builder->CreateICmpUGE(lhs, rhs, "uicmp");
    case BinOpNode::GT: return builder->CreateICmpUGT(lhs, rhs, "uicmp");
    case BinOpNode::NE: return builder->CreateICmpNE(lhs, rhs, "uicmp");
    case BinOpNode::BITWISE_AND: return builder->CreateAnd(lhs, rhs, "uiand");
    case BinOpNode::BITWISE_XOR: return builder->CreateXor(lhs, rhs, "xor");
    case BinOpNode::BITWISE_OR: return builder->CreateOr(lhs, rhs, "or");
    default: assert(false); return 0;
  }
}

llvm::Value* GenerateBinOpFloat(LLVMBuilder*   builder,
                                BinOpNode::Op  op,
                                llvm::Value*   lhs,
                                llvm::Value*   rhs) {
  switch (op) {
    case BinOpNode::ADD: return builder->CreateFAdd(lhs, rhs, "fadd");
    case BinOpNode::SUB: return builder->CreateFSub(lhs, rhs, "fsub");
    case BinOpNode::MUL: return builder->CreateFMul(lhs, rhs, "fmul");
    case BinOpNode::DIV: return builder->CreateFDiv(lhs, rhs, "fdiv");
    case BinOpNode::LT: return builder->CreateFCmpOLT(lhs, rhs, "fcmp");
    case BinOpNode::LE: return builder->CreateFCmpOLE(lhs, rhs, "fcmp");
    case BinOpNode::EQ: return builder->CreateFCmpOEQ(lhs, rhs, "fcmp");
    case BinOpNode::GE: return builder->CreateFCmpOGE(lhs, rhs, "fcmp");
    case BinOpNode::GT: return builder->CreateFCmpOGT(lhs, rhs, "fcmp");
    case BinOpNode::NE: return builder->CreateFCmpONE(lhs, rhs, "fcmp");
    case BinOpNode::MOD: return builder->CreateFRem(lhs, rhs, "frem");
    default: assert(false); return 0;
  }
}

}

CodeGenLLVM::CodeGenLLVM(llvm::LLVMContext*                 context,
                         TypeTable*                         types,
                         llvm::Module*                      module,
                         LLVMBuilder*                       builder,
                         llvm::legacy::FunctionPassManager* fpm)
    : types_(types),
      context_(context),
      module_(module),
      builder_(builder),
      fpm_(fpm),
      debugOutput_(false) {
  boolType_ = llvm::Type::getInt1Ty(*context_);
  intType_ = llvm::Type::getInt32Ty(*context_);
  floatType_ = llvm::Type::getFloatTy(*context_);
  doubleType_ = llvm::Type::getDoubleTy(*context_);
  byteType_ = llvm::Type::getInt8Ty(*context_);
  shortType_ = llvm::Type::getInt16Ty(*context_);
  llvm::Type* voidType = llvm::Type::getVoidTy(*context_);
  ptrType_ = llvm::PointerType::get(*context_, 0);
  deleterType_ = llvm::FunctionType::get(voidType, { ptrType_ }, false);
#if TARGET_OS_IS_WIN && TARGET_CPU_IS_X86
  freeFunc_ = module_->getOrInsertFunction("_aligned_free", deleterType_);
#else
  freeFunc_ = module_->getOrInsertFunction("free", deleterType_);
#endif
  controlBlockType_ = ControlBlockType();
  typeList_ = new llvm::GlobalVariable(
      *module_, ptrType_, true, llvm::GlobalVariable::ExternalLinkage, nullptr, "_type_list");
}

void CodeGenLLVM::Run(Stmts* stmts) {
  // An iterator can't be used here, since new types may be added (but won't need codegen).
  int size = types_->GetTypes().size();
  for (int i = 0; i < size; ++i) {
    Type* type = types_->GetTypes()[i];
    if (type->IsClass()) {
      ClassType* classType = static_cast<ClassType*>(type);
      if (!classType->IsFullySpecified()) { continue; }
      for (const auto& mit : classType->GetMethods()) {
        GenCodeForMethod(mit.get());
      }
    }
  }
  stmts->Accept(this);
}

llvm::Type* CodeGenLLVM::PadType(llvm::Type* type, int padding) {
  if (padding == 0) return type;

  std::vector<llvm::Type*> types;
  types.push_back(type);
  types.push_back(llvm::ArrayType::get(byteType_, padding));
  return llvm::StructType::get(*context_, types);
}

void CodeGenLLVM::ConvertAndAppendFieldTypes(ClassType*                classType,
                                             std::vector<llvm::Type*>* types) {
  if (classType->GetParent()) { ConvertAndAppendFieldTypes(classType->GetParent(), types); }
  for (const auto& field : classType->GetFields()) {
    types->push_back(PadType(ConvertType(field->type), field->padding));
  }
  if (int padding = classType->GetPadding()) {
    types->push_back(llvm::ArrayType::get(byteType_, padding));
  }
}

llvm::Type* CodeGenLLVM::ControlBlockType() {
  std::vector<llvm::Type*> types;
  types.push_back(intType_);      // strong refcount
  types.push_back(intType_);      // weak refcount
  types.push_back(intType_);      // array length
  types.push_back(ptrType_);      // ClassType
  types.push_back(ptrType_);      // deleter
  return llvm::StructType::get(*context_, types);
}

llvm::Type* CodeGenLLVM::ConvertType(Type* type) {
  if (type->IsStrongPtr() || type->IsWeakPtr()) {
    std::vector<llvm::Type*> types;
    Type*                    baseType = static_cast<PtrType*>(type)->GetBaseType();
    baseType = baseType->GetUnqualifiedType();
    types.push_back(ptrType_); // Ptr
    types.push_back(ptrType_); // Control block
    return llvm::StructType::get(*context_, types);
  } else if (type->IsRawPtr()) {
    Type* baseType = static_cast<RawPtrType*>(type)->GetBaseType();
    baseType = baseType->GetUnqualifiedType();
    if (baseType->IsUnsizedArray() || baseType->IsUnsizedClass()) {
      std::vector<llvm::Type*> types;
      types.push_back(ptrType_); // Ptr
      types.push_back(intType_); // Length
      return llvm::StructType::get(*context_, types);
    } else {
      return ptrType_;
    }
  } else if (type->IsArray()) {
    ArrayType* atype = static_cast<ArrayType*>(type);
    return llvm::ArrayType::get(ConvertArrayElementType(atype), atype->GetNumElements());
  } else if (type->IsClass()) {
    ClassType*               classType = static_cast<ClassType*>(type);
    if (auto placeholder = classPlaceholders_[classType]) {
      return placeholder;
    }
    classPlaceholders_[classType] = llvm::StructType::get(*context_);
    std::vector<llvm::Type*> types;
    ConvertAndAppendFieldTypes(classType, &types);
    classPlaceholders_[classType] = nullptr;
    return llvm::StructType::get(*context_, types);
  } else if (type->IsByte() || type->IsUByte()) {
    return byteType_;
  } else if (type->IsShort() || type->IsUShort()) {
    return shortType_;
  } else if (type->IsInt() || type->IsUInt() || type->IsEnum()) {
    return intType_;
  } else if (type->IsFloat()) {
    return floatType_;
  } else if (type->IsDouble()) {
    return doubleType_;
  } else if (type->IsBool()) {
    return boolType_;
  } else if (type->IsVector()) {
    VectorType* v = static_cast<VectorType*>(type);
    llvm::Type* componentType = ConvertType(v->GetElementType());
    return llvm::VectorType::get(componentType, v->GetNumElements(), false);
  } else if (type->IsMatrix()) {
    MatrixType* m = static_cast<MatrixType*>(type);
    return llvm::ArrayType::get(ConvertType(m->GetColumnType()), m->GetNumColumns());
  } else if (type->IsVoid()) {
    return llvm::Type::getVoidTy(*context_);
  } else if (type->IsQualified()) {
    return ConvertType(static_cast<QualifiedType*>(type)->GetBaseType());
  } else {
    assert("ConvertType:  unknown type" == 0);
    return 0;
  }
}

llvm::Type* CodeGenLLVM::ConvertArrayElementType(ArrayType* arrayType) {
  Type*       elementType = arrayType->GetElementType();
  llvm::Type* result = ConvertType(arrayType->GetElementType());
  return PadType(result, arrayType->GetElementPadding());
}

llvm::Type* CodeGenLLVM::ConvertTypeToNative(Type* type) {
  if (type->IsPtr() || type->IsVector()) {
    return ptrType_;
  }
  return ConvertType(type);
}

llvm::Constant* CodeGenLLVM::Int(int value) { return llvm::ConstantInt::get(intType_, value); }

llvm::Value* CodeGenLLVM::CreateTypePtr(Type* type) {
  llvm::Value* ptr = builder_->CreateLoad(ptrType_, typeList_);
  llvm::Value* typeID = typeMap_[type];
  if (!typeID) {
    typeID = Int(referencedTypes_.size());
    typeMap_[type] = typeID;
    referencedTypes_.push_back(type);
  }
  ptr = builder_->CreateGEP(ptrType_, ptr, {typeID});
  return builder_->CreateLoad(ptrType_, ptr);
}

llvm::Value* CodeGenLLVM::CreateControlBlock(Type* type) {
  llvm::Value* controlBlock = CreateMalloc(controlBlockType_, 0);
  builder_->CreateStore(Int(1), GetStrongRefCountAddress(controlBlock));
  builder_->CreateStore(Int(1), GetWeakRefCountAddress(controlBlock));
  int arrayLength = type->IsArray() ? static_cast<ArrayType*>(type)->GetNumElements() : 0;
  builder_->CreateStore(Int(arrayLength), GetArrayLengthAddress(controlBlock));
  builder_->CreateStore(CreateTypePtr(type), GetClassTypeAddress(controlBlock));
  type = type->GetUnqualifiedType();
  builder_->CreateStore(GetOrCreateDeleter(type), GetDeleterAddress(controlBlock));
  return controlBlock;
}

llvm::Value* CodeGenLLVM::CreatePointer(llvm::Value* obj, llvm::Value* controlBlockOrLength) {
  std::vector<llvm::Type*> types;
  types.push_back(obj->getType());
  types.push_back(controlBlockOrLength->getType());
  llvm::Type*  type = llvm::StructType::get(*context_, types);
  llvm::Value* result = llvm::ConstantAggregateZero::get(type);
  result = builder_->CreateInsertValue(result, obj, 0);
  result = builder_->CreateInsertValue(result, controlBlockOrLength, 1);
  return result;
}

llvm::Value* CodeGenLLVM::GetStrongRefCountAddress(llvm::Value* controlBlock) {
  return builder_->CreateGEP(controlBlockType_, controlBlock, {Int(0), Int(0)});
}

llvm::Value* CodeGenLLVM::GetWeakRefCountAddress(llvm::Value* controlBlock) {
  return builder_->CreateGEP(controlBlockType_, controlBlock, {Int(0), Int(1)});
}

llvm::Value* CodeGenLLVM::GetArrayLengthAddress(llvm::Value* controlBlock) {
  return builder_->CreateGEP(controlBlockType_, controlBlock, {Int(0), Int(2)});
}

llvm::Value* CodeGenLLVM::GetClassTypeAddress(llvm::Value* controlBlock) {
  return builder_->CreateGEP(controlBlockType_, controlBlock, {Int(0), Int(3)});
}

llvm::Value* CodeGenLLVM::GetDeleterAddress(llvm::Value* controlBlock) {
  return builder_->CreateGEP(controlBlockType_, controlBlock, {Int(0), Int(4)});
}

llvm::BasicBlock* CodeGenLLVM::NullControlBlockCheck(llvm::Value* controlBlock, BinOpNode::Op op) {
  llvm::Value*      nullControlBlock = llvm::ConstantPointerNull::get(ptrType_);
  llvm::Value*      condition = GenerateBinOpInt(builder_, op, controlBlock, nullControlBlock);
  llvm::BasicBlock* trueBlock = CreateBasicBlock("trueBlock");
  llvm::BasicBlock* afterBlock = CreateBasicBlock("after");
  builder_->CreateCondBr(condition, trueBlock, afterBlock);
  builder_->SetInsertPoint(trueBlock);
  return afterBlock;
}

void CodeGenLLVM::RefStrongPtr(llvm::Value* ptr) {
  llvm::Value*      controlBlock = builder_->CreateExtractValue(ptr, {1});
  llvm::BasicBlock* afterBlock = NullControlBlockCheck(controlBlock, BinOpNode::NE);

  llvm::Value* address = GetStrongRefCountAddress(controlBlock);
  llvm::Value* refCount = builder_->CreateLoad(intType_, address);
  refCount = builder_->CreateAdd(refCount, Int(1));
  builder_->CreateStore(refCount, address);

  builder_->CreateBr(afterBlock);
  builder_->SetInsertPoint(afterBlock);
  RefWeakPtr(ptr);
}

llvm::BasicBlock* CodeGenLLVM::CreateBasicBlock(const char* name) {
  return llvm::BasicBlock::Create(*context_, name, builder_->GetInsertBlock()->getParent());
}

void CodeGenLLVM::UnrefStrongPtr(llvm::Value* ptr, StrongPtrType* type) {
  llvm::Value*      controlBlock = builder_->CreateExtractValue(ptr, {1});
  llvm::BasicBlock* afterBlock = NullControlBlockCheck(controlBlock, BinOpNode::NE);

  llvm::Value* address = GetStrongRefCountAddress(controlBlock);
  llvm::Value* refCount = builder_->CreateLoad(intType_, address);
  refCount = builder_->CreateSub(refCount, Int(1));
  builder_->CreateStore(refCount, address);
  llvm::Value*      isZero = builder_->CreateICmpEQ(refCount, Int(0));
  llvm::BasicBlock* trueBlock = CreateBasicBlock("trueBlock");
  builder_->CreateCondBr(isZero, trueBlock, afterBlock);
  builder_->SetInsertPoint(trueBlock);
  Type* baseType = type->GetBaseType()->GetUnqualifiedType();
  llvm::Value* arg = builder_->CreateExtractValue(ptr, {0});
  llvm::Value* deleter = builder_->CreateLoad(ptrType_, GetDeleterAddress(controlBlock));
  builder_->CreateCall(deleterType_, deleter, {arg});
  builder_->CreateBr(afterBlock);
  builder_->SetInsertPoint(afterBlock);
  UnrefWeakPtr(ptr);
}

llvm::AllocaInst* CodeGenLLVM::CreateEntryBlockAlloca(llvm::Function* function, Var* var) {
  LLVMBuilder builder(&function->getEntryBlock(), function->getEntryBlock().begin());
  return allocas_[var] = builder.CreateAlloca(ConvertType(var->type), 0, var->name.c_str());
}

void CodeGenLLVM::RefWeakPtr(llvm::Value* ptr) {
  llvm::Value*      controlBlock = builder_->CreateExtractValue(ptr, {1});
  llvm::BasicBlock* afterBlock = NullControlBlockCheck(controlBlock, BinOpNode::NE);

  llvm::Value* address = GetWeakRefCountAddress(controlBlock);
  llvm::Value* refCount = builder_->CreateLoad(intType_, address);
  refCount = builder_->CreateAdd(refCount, Int(1));
  builder_->CreateStore(refCount, address);

  builder_->CreateBr(afterBlock);
  builder_->SetInsertPoint(afterBlock);
}

void CodeGenLLVM::UnrefWeakPtr(llvm::Value* ptr) {
  llvm::Value*      controlBlock = builder_->CreateExtractValue(ptr, {1});
  llvm::BasicBlock* afterBlock = NullControlBlockCheck(controlBlock, BinOpNode::NE);

  llvm::Value* address = GetWeakRefCountAddress(controlBlock);
  llvm::Value* refCount = builder_->CreateLoad(intType_, address);
  refCount = builder_->CreateSub(refCount, Int(1));
  builder_->CreateStore(refCount, address);
  llvm::Value*      isZero = builder_->CreateICmpEQ(refCount, Int(0));
  llvm::BasicBlock* trueBlock = CreateBasicBlock("trueBlock");
  builder_->CreateCondBr(isZero, trueBlock, afterBlock);
  builder_->SetInsertPoint(trueBlock);
  builder_->CreateCall(freeFunc_, controlBlock);

  builder_->CreateBr(afterBlock);
  builder_->SetInsertPoint(afterBlock);
}

llvm::Value* CodeGenLLVM::GetOrCreateDeleter(Type* type) {
  if (!type->NeedsDestruction()) return freeFunc_.getCallee();
  if (type->IsClass()) {
    auto destructor = static_cast<ClassType*>(type)->GetDestructor();
    if (destructor && destructor->IsNative()) {
      // Native destructors will handle freeing, so just return the destructor.
      return GetOrCreateMethodStub(destructor);
    }
  }
  if (auto deleter = deleters_[type]) { return deleter; }
  auto deleter = llvm::Function::Create(deleterType_, llvm::GlobalValue::InternalLinkage,
                                        "__deleter", module_);
  llvm::BasicBlock* whereWasI = builder_->GetInsertBlock();
  llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context_, "entry", deleter);
  builder_->SetInsertPoint(entry);
  llvm::Value* value = &*deleter->arg_begin();
  Destroy(type, value);
  builder_->CreateCall(freeFunc_, value);
  builder_->CreateRet(nullptr);
  builder_->SetInsertPoint(whereWasI);
  fpm_->run(*deleter);
  return deleters_[type] = deleter;
}

llvm::Function* CodeGenLLVM::GetOrCreateMethodStub(Method* method) {
  if (auto function = functions_[method]) { return function; }
  std::vector<llvm::Type*> params;
  llvm::Intrinsic::ID      intrinsic = llvm::Intrinsic::not_intrinsic;
  bool skipFirst = false;
  if (method->IsNative()) {
    if (method->templateMethod) { return GetOrCreateMethodStub(method->templateMethod); }
    if (method->IsConstructor()) {
      skipFirst = true;
      if (method->classType->IsClassTemplate()) {
        // First argument is the storage qualifier (as uint)
        params.push_back(intType_);
        ClassTemplate* classTemplate = static_cast<ClassTemplate*>(method->classType);
        for (Type* const& type : classTemplate->GetFormalTemplateArgs()) {
          params.push_back(ptrType_);
        }
      }
    }
    intrinsic = FindIntrinsic(method);
  }
  bool nativeTypes = method->IsNative() && !intrinsic;
  for (const auto& it : method->formalArgList) {
    if (skipFirst) { skipFirst = false; continue; }
    Var* var = it.get();
    if (nativeTypes) {
      params.push_back(ConvertTypeToNative(var->type));
    } else {
      params.push_back(ConvertType(var->type));
    }
  }
  llvm::Function* function;
  if (intrinsic) {
    function = llvm::Intrinsic::getOrInsertDeclaration(module_, intrinsic, params);
    if (intrinsic == llvm::Intrinsic::ctlz) {
      // is_zero_poison
      params.push_back(boolType_);
    }
  } else {
    llvm::Type* returnType =
        nativeTypes ? ConvertTypeToNative(method->returnType) : ConvertType(method->returnType);
    llvm::FunctionType* functionType = llvm::FunctionType::get(returnType, params, false);
    function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage,
                                      method->GetMangledName(), module_);
  }

  return functions_[method] = function;
}

llvm::Intrinsic::ID CodeGenLLVM::FindIntrinsic(Method* method) {
  if (method->formalArgList.empty()) return llvm::Intrinsic::not_intrinsic;

  // All intrisics are Math functions, currently.
  if (method->classType->GetName() != "Math") return llvm::Intrinsic::not_intrinsic;
  Type* argType = method->formalArgList[0]->type;

  auto findIntrinsic = [](Method* method, std::span<const Intrinsic> intrinsics) -> llvm::Intrinsic::ID {
    for (auto intrinsic : intrinsics) {
      if (method->name == intrinsic.methodName) return intrinsic.id;
    }
    return llvm::Intrinsic::not_intrinsic;
  };

  if (argType->IsFloat() || argType->IsFloatVector()) {
    if (auto id = findIntrinsic(method, floatIntrinsics)) return id;
  } else if (argType->IsBool() || argType->IsBoolVector()) {
    if (auto id = findIntrinsic(method, boolIntrinsics)) return id;
  } else if (argType->IsInteger() || argType->IsIntegerVector()) {
    if (argType->IsUnsigned()) {
      if (auto id = findIntrinsic(method, uintIntrinsics)) return id;
    } else {
      if (auto id = findIntrinsic(method, intIntrinsics)) return id;
    }
  }
  return llvm::Intrinsic::not_intrinsic;
}

llvm::Value* CodeGenLLVM::GetSourceFile(const FileLocation& location) {
  const std::string* filename = location.filename.get();
  Type*              type = types_->GetArrayType(types_->GetUByte(), 0, MemoryLayout::Default);
  return GenerateGlobalData(filename->c_str(), filename->length(), type);
}

llvm::Value* CodeGenLLVM::GetSourceLine(const FileLocation& location) {
  return llvm::ConstantInt::get(intType_, location.lineNum, true);
}

BuiltinCall CodeGenLLVM::FindBuiltin(Method* method) {
  constexpr struct {
    const char* className;
    const char* methodName;
    BuiltinCall call;
  } builtins[] = {
      "System", "GetSourceFile", &CodeGenLLVM::GetSourceFile,
      "System", "GetSourceLine", &CodeGenLLVM::GetSourceLine,
  };

  for (auto builtin : builtins) {
    if (method->name == builtin.methodName && method->classType->GetName() == builtin.className) {
      return builtin.call;
    }
  }
  return nullptr;
}

void CodeGenLLVM::GenCodeForMethod(Method* method) {
  if ((method->modifiers & (Method::Modifier::Vertex | Method::Modifier::Fragment | Method::Modifier::Compute)) != 0) {
    CodeGenSPIRV codeGenSPIRV(types_);
    codeGenSPIRV.Run(method);
    std::vector<uint32_t> spirv;
    spirv = codeGenSPIRV.header();
    spirv.insert(spirv.end(), codeGenSPIRV.annotations().begin(), codeGenSPIRV.annotations().end());
    spirv.insert(spirv.end(), codeGenSPIRV.decl().begin(), codeGenSPIRV.decl().end());
    spirv.insert(spirv.end(), codeGenSPIRV.GetBody().begin(), codeGenSPIRV.GetBody().end());

#if TARGET_OS_IS_WASM
    tint::spirv::reader::Options spirvOptions;
    tint::Program                program = tint::spirv::reader::Read(spirv, spirvOptions);
    if (!program.IsValid()) {
      std::cerr << "Tint SPIR-V reader failure:\n" << program.Diagnostics() << "\n";
      return;
    }
    tint::wgsl::writer::Options wgslOptions;
    auto                        result = tint::wgsl::writer::Generate(program, wgslOptions);
    if (result != tint::Success) {
      std::cerr << "Tint WGSL writer failure:\n" << result.Failure() << "\n";
      return;
    }
    method->wgsl = result.Get().wgsl;
#else
    method->spirv = spirv;
#endif
    return;
  }
  if (method->modifiers & Method::Modifier::DeviceOnly) { return; }
  llvm::Function* function = GetOrCreateMethodStub(method);
  if (method->IsNative()) return;
  llvm::BasicBlock* whereWasI = builder_->GetInsertBlock();
  llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context_, "entry", function);
  builder_->SetInsertPoint(entry);
  VarVector::iterator          it;
  llvm::Function::arg_iterator ai;
  // Name the arguments.
  for (ai = function->arg_begin(), it = method->formalArgList.begin();
       ai != function->arg_end() && it != method->formalArgList.end(); ai++, it++) {
    Var* var = it->get();
    ai->setName(var->name.c_str());
    auto allocaInst = CreateEntryBlockAlloca(function, var);
    builder_->CreateStore(&*ai, allocaInst);
  }
  method->stmts->Accept(this);
  fpm_->run(*function);
  builder_->SetInsertPoint(whereWasI);
#if !defined(NDEBUG)
//  if (debugOutput_) function->dump();
#endif
}

llvm::Value* CodeGenLLVM::CreateMalloc(llvm::Type* type, llvm::Value* arraySize) {
  // TODO(senorblanco):  initialize this once, not every time
  std::vector<llvm::Type*> args;
  args.push_back(intType_);
#if TARGET_OS_IS_WIN && TARGET_CPU_IS_X86
  args.push_back(intType_);
#endif
  llvm::FunctionType* ft = llvm::FunctionType::get(ptrType_, args, false);
  llvm::Value*        indices[] = {arraySize ? arraySize : llvm::ConstantInt::get(intType_, 1)};
  llvm::Value*        nullPtr = llvm::ConstantPointerNull::get(ptrType_);
  llvm::Value*        size = builder_->CreateGEP(type, nullPtr, indices);
  llvm::Value*        sizeInt = builder_->CreatePtrToInt(size, intType_);
#if TARGET_OS_IS_WIN && TARGET_CPU_IS_X86
  llvm::FunctionCallee alignedMalloc = module_->getOrInsertFunction("_aligned_malloc", ft);
  llvm::Value*         sixteen = llvm::ConstantInt::get(intType_, 16);
  llvm::Value*         ptr = builder_->CreateCall(alignedMalloc, {sizeInt, sixteen});
#else
  llvm::FunctionCallee malloc = module_->getOrInsertFunction("malloc", ft);
  llvm::Value*         ptr = builder_->CreateCall(malloc, sizeInt);
#endif
  return builder_->CreateBitCast(ptr, ptrType_);
}

llvm::Value* CodeGenLLVM::GenerateLLVM(Expr* expr) {
  if (auto result = exprCache_[expr]) { return result; }
  auto value = static_cast<llvm::Value*>(std::get<void*>(expr->Accept(this)));
  return exprCache_[expr] = value;
}

llvm::Value* CodeGenLLVM::ConvertToNative(Type* type, llvm::Value* value) {
  if (type->IsStrongPtr() || type->IsWeakPtr() || type->IsVector() ||
      (type->IsRawPtr() && static_cast<RawPtrType*>(type)->GetBaseType()->IsUnsizedArray())) {
    // All types that can't be passed through native function calls are spilled to the stack.
    // Arrays are passed as Array*, Smart ptrs are passed as Object*, and vectors as component*.
    llvm::Value* alloc = builder_->CreateAlloca(ConvertType(type));
    builder_->CreateStore(value, alloc);
    value = alloc;
  }
  return value;
}

llvm::Value* CodeGenLLVM::ConvertFromNative(Type* type, llvm::Value* value) {
  if (type->IsStrongPtr() || type->IsWeakPtr()) {
    Type* baseType = static_cast<PtrType*>(type)->GetBaseType();
    Type* unqualifiedType = baseType->GetUnqualifiedType();
    if (unqualifiedType->IsClass() && static_cast<ClassType*>(unqualifiedType)->HasNativeMethods()) {
      return CreatePointer(value, CreateControlBlock(baseType));
    } else {
      // Dereference Object*.
      value = builder_->CreateLoad(ConvertType(type), value);
    }
  } else if (type->IsVector()) {
    // Dereference vector*
    value = builder_->CreateLoad(ConvertType(type), value);
  }
  return value;
}

llvm::Value* CodeGenLLVM::GenerateTranspose(llvm::Value* srcMatrix, MatrixType* srcMatrixType) {
  VectorType* srcColumnType = srcMatrixType->GetColumnType();
  unsigned    numSrcColumns = srcMatrixType->GetNumColumns();
  unsigned    numSrcRows = srcColumnType->GetNumElements();

  VectorType* dstColumnType = types_->GetVector(srcColumnType->GetElementType(), numSrcColumns);
  MatrixType* dstMatrixType = types_->GetMatrix(dstColumnType, numSrcRows);

  // Extract all the columns of the src matrix.
  std::vector<llvm::Value*> srcColumns(numSrcColumns);
  for (unsigned col = 0; col < numSrcColumns; ++col) {
    srcColumns[col] = builder_->CreateExtractValue(srcMatrix, {col});
  }

  llvm::Value* dstMatrix = llvm::ConstantAggregateZero::get(ConvertType(dstMatrixType));
  // Create a column in the dst matrix for each row in the src matrix.
  for (unsigned col = 0; col < numSrcRows; ++col) {
    llvm::Value* dstColumn = llvm::ConstantAggregateZero::get(ConvertType(dstColumnType));
    for (unsigned row = 0; row < numSrcColumns; ++row) {
      llvm::Value* value = builder_->CreateExtractElement(srcColumns[row], Int(col));
      dstColumn = builder_->CreateInsertElement(dstColumn, value, Int(row));
    }
    dstMatrix = builder_->CreateInsertValue(dstMatrix, dstColumn, {col});
  }
  return dstMatrix;
}

llvm::Value* CodeGenLLVM::GenerateDotProduct(llvm::Value* lhs, llvm::Value* rhs) {
  unsigned length = llvm::cast<llvm::FixedVectorType>(lhs->getType())->getNumElements();
  llvm::Value* product = builder_->CreateFMul(lhs, rhs);
  llvm::Value* sum = builder_->CreateExtractElement(product, Int(0));
  for (unsigned i = 1; i < length; ++i) {
    llvm::Value* value = builder_->CreateExtractElement(product, Int(i));
    sum = builder_->CreateFAdd(sum, value);
  }
  return sum;
}

llvm::Value* CodeGenLLVM::GenerateCrossProduct(llvm::Value* lhs, llvm::Value* rhs) {
  assert(llvm::cast<llvm::FixedVectorType>(lhs->getType())->getNumElements() == 3);
  llvm::Value* dst = llvm::ConstantAggregateZero::get(lhs->getType());
  for (int i = 0; i < 3; ++i) {
    llvm::Value* l1 = builder_->CreateExtractElement(lhs, Int((i + 1) % 3));
    llvm::Value* l2 = builder_->CreateExtractElement(lhs, Int((i + 2) % 3));
    llvm::Value* r1 = builder_->CreateExtractElement(rhs, Int((i + 1) % 3));
    llvm::Value* r2 = builder_->CreateExtractElement(rhs, Int((i + 2) % 3));
    llvm::Value* product1 = builder_->CreateFMul(l1, r2);
    llvm::Value* product2 = builder_->CreateFMul(l2, r1);
    llvm::Value* result = builder_->CreateFSub(product1, product2);
    dst = builder_->CreateInsertElement(dst, result, Int(i));
  }
  return dst;
}

llvm::Value* CodeGenLLVM::GenerateVectorLength(llvm::Value* value) {
  auto dotProduct = GenerateDotProduct(value, value);
  std::vector<llvm::Type*> params;
  auto type = value->getType()->getContainedType(0);
  auto function = llvm::Intrinsic::getOrInsertDeclaration(module_, llvm::Intrinsic::sqrt, {type});
  return builder_->CreateCall(function, {dotProduct});
}

llvm::Value* CodeGenLLVM::GenerateVectorNormalize(llvm::Value* value) {
  auto length = GenerateVectorLength(value);
  auto one = llvm::ConstantFP::get(floatType_, 1.0);
  auto recip = builder_->CreateFDiv(one, length);
  unsigned numElts = llvm::cast<llvm::FixedVectorType>(value->getType())->getNumElements();
  auto scale = builder_->CreateVectorSplat(numElts, recip);
  return builder_->CreateFMul(value, scale);
}

llvm::Value* CodeGenLLVM::GenerateMatrixMultiply(llvm::Value* lhs,
                                                 llvm::Value* rhs,
                                                 MatrixType*  lhsType,
                                                 MatrixType*  rhsType) {
  llvm::Value* lhsT = GenerateTranspose(lhs, lhsType);
  assert(lhsT->getType() == rhs->getType());

  VectorType* columnType = rhsType->GetColumnType();
  llvm::Type* columnTypeLLVM = ConvertType(columnType);
  llvm::Type* matrixTypeLLVM = ConvertType(rhsType);

  unsigned numColumns = rhsType->GetNumColumns();
  unsigned numRows = columnType->GetNumElements();

  std::vector<llvm::Value*> lhsRows(numRows);
  for (unsigned row = 0; row < numRows; ++row) {
    lhsRows[row] = builder_->CreateExtractValue(lhsT, row);
  }
  llvm::Value* dstMatrix = llvm::ConstantAggregateZero::get(matrixTypeLLVM);
  for (unsigned col = 0; col < numColumns; ++col) {
    llvm::Value* rhsCol = builder_->CreateExtractValue(rhs, col);
    llvm::Value* dstColumn = llvm::ConstantAggregateZero::get(columnTypeLLVM);
    for (unsigned row = 0; row < numRows; ++row) {
      llvm::Value* value = GenerateDotProduct(lhsRows[row], rhsCol);
      dstColumn = builder_->CreateInsertElement(dstColumn, value, Int(row));
    }
    dstMatrix = builder_->CreateInsertValue(dstMatrix, dstColumn, {col});
  }
  return dstMatrix;
}

llvm::Value* CodeGenLLVM::GenerateBinOp(BinOpNode*   node,
                                        llvm::Value* lhs,
                                        llvm::Value* rhs,
                                        Type*        type) {
  if (type->IsInteger() || type->IsIntegerVector() || type->IsBool() || type->IsEnum()) {
    if (type->IsUnsigned()) {
      return GenerateBinOpUInt(builder_, node->GetOp(), lhs, rhs);
    } else {
      return GenerateBinOpInt(builder_, node->GetOp(), lhs, rhs);
    }
  } else if (type->IsFloatingPoint() || type->IsFloatVector()) {
    return GenerateBinOpFloat(builder_, node->GetOp(), lhs, rhs);
  } else if (type->IsMatrix() && node->GetOp() == BinOpNode::MUL) {
    auto matrixType = static_cast<MatrixType*>(type);
    return GenerateMatrixMultiply(lhs, rhs, matrixType, matrixType);
  } else if (type->IsStrongPtr() || type->IsWeakPtr()) {
    lhs = builder_->CreateExtractValue(lhs, {0});
    rhs = builder_->CreateExtractValue(rhs, {0});
    return GenerateBinOpInt(builder_, node->GetOp(), lhs, rhs);
  } else {
    assert(false);
    return nullptr;
  }
}

Result CodeGenLLVM::Visit(BinOpNode* node) {
  llvm::Value* lhs = GenerateLLVM(node->GetLHS());
  llvm::Value* rhs = GenerateLLVM(node->GetRHS());
  Type*        lhsType = node->GetLHS()->GetType(types_);
  Type*        rhsType = node->GetRHS()->GetType(types_);
  llvm::Value* ret = nullptr;
  if (lhsType == rhsType) {
    ret = GenerateBinOp(node, lhs, rhs, lhsType);
  } else if (lhsType->CanWidenTo(rhsType)) {
    ret = GenerateBinOp(node, lhs, rhs, rhsType);
  } else if (rhsType->CanWidenTo(lhsType)) {
    ret = GenerateBinOp(node, lhs, rhs, lhsType);
  } else if (TypeTable::VectorScalar(lhsType, rhsType)) {
    VectorType* lhsVectorType = static_cast<VectorType*>(lhsType);
    if (lhsVectorType->GetElementType() == rhsType) {
      rhs = builder_->CreateVectorSplat(lhsVectorType->GetNumElements(), rhs);
      ret = GenerateBinOp(node, lhs, rhs, lhsType);
    } else {
      assert(false);
    }
  } else if (TypeTable::ScalarVector(lhsType, rhsType)) {
    VectorType* rhsVectorType = static_cast<VectorType*>(rhsType);
    if (rhsVectorType->GetElementType() == lhsType) {
      lhs = builder_->CreateVectorSplat(rhsVectorType->GetNumElements(), lhs);
      ret = GenerateBinOp(node, lhs, rhs, lhsType);
    } else {
      assert(false);
    }
  } else if (TypeTable::MatrixVector(lhsType, rhsType)) {
    auto         matrixType = static_cast<MatrixType*>(lhsType);
    auto         vectorType = static_cast<VectorType*>(rhsType);
    auto         columnType = matrixType->GetColumnType();
    llvm::Value* lhsT = GenerateTranspose(lhs, matrixType);
    ret = llvm::ConstantAggregateZero::get(ConvertType(vectorType));
    for (int i = 0; i < vectorType->GetNumElements(); i += 1) {
      llvm::Value* row = builder_->CreateExtractValue(lhsT, i);
      llvm::Value* value = GenerateDotProduct(row, rhs);
      ret = builder_->CreateInsertElement(ret, value, Int(i));
    }
  } else {
    assert(false);
  }
  return ret;
}

llvm::Value* CodeGenLLVM::CreateCast(Type*        srcType,
                                     Type*        dstType,
                                     llvm::Value* value,
                                     llvm::Type*  dstLLVMType) {
  if (srcType->IsInteger()) {
    IntegerType* srcIntegerType = static_cast<IntegerType*>(srcType);
    if (dstType->IsFloatingPoint()) {
      if (srcIntegerType->Signed()) {
        return builder_->CreateSIToFP(value, dstLLVMType);
      } else {
        return builder_->CreateUIToFP(value, dstLLVMType);
      }
    } else if (dstType->IsInteger()) {
      IntegerType* dstIntegerType = static_cast<IntegerType*>(dstType);
      if (srcIntegerType->GetBits() < dstIntegerType->GetBits()) {
        if (dstIntegerType->Signed()) {
          return builder_->CreateSExt(value, dstLLVMType);
        } else {
          return builder_->CreateZExt(value, dstLLVMType);
        }
      } else if (srcIntegerType->GetBits() > dstIntegerType->GetBits()) {
        return builder_->CreateTrunc(value, dstLLVMType);
      } else {
        return value;
      }
    }
  } else if (srcType->IsFloatingPoint()) {
    FloatingPointType* srcFPType = static_cast<FloatingPointType*>(srcType);
    if (dstType->IsInteger()) {
      IntegerType* dstIntegerType = static_cast<IntegerType*>(dstType);
      if (dstIntegerType->Signed()) {
        return builder_->CreateFPToSI(value, dstLLVMType);
      } else {
        return builder_->CreateFPToUI(value, dstLLVMType);
      }
    } else if (dstType->IsFloatingPoint()) {
      int srcBits = static_cast<FloatingPointType*>(srcType)->GetBits();
      int dstBits = static_cast<FloatingPointType*>(dstType)->GetBits();
      if (srcBits > dstBits) {
        return builder_->CreateFPTrunc(value, dstLLVMType);
      } else if (srcBits < dstBits) {
        return builder_->CreateFPExt(value, dstLLVMType);
      } else {
        return value;
      }
    }
  } else if (srcType->IsEnum() && (dstType->IsInteger())) {
    return value;
  } else if (srcType->IsVector() && dstType->IsVector()) {
    return CreateCast(static_cast<VectorType*>(srcType)->GetElementType(),
                      static_cast<VectorType*>(dstType)->GetElementType(), value,
                      ConvertType(dstType));
  } else if (srcType->IsPtr() && dstType->IsPtr()) {
    if (srcType->IsStrongPtr() && dstType->IsWeakPtr()) {
      RefWeakPtr(value);
      AppendTemporary(value, static_cast<StrongPtrType*>(srcType));
    }
    Type*        dstBase = static_cast<PtrType*>(dstType)->GetBaseType();
    llvm::Value* ptr = builder_->CreateExtractValue(value, {0});
    llvm::Value* controlBlock = builder_->CreateExtractValue(value, {1});
    llvm::Value* newPtr = builder_->CreateBitCast(ptr, ptrType_);
    return CreatePointer(newPtr, controlBlock);
  }
  assert(!"unimplemented cast");
  return nullptr;
}

Result CodeGenLLVM::Visit(CastExpr* expr) {
  Type* dstType = expr->GetType();
  Type* srcType = expr->GetExpr()->GetType(types_);
  assert(srcType != dstType);
  llvm::Value* value = GenerateLLVM(expr->GetExpr());
  return CreateCast(srcType, dstType, value, ConvertType(dstType));
}

llvm::Value* CodeGenLLVM::GenerateGlobalData(const void* data, size_t size, Type* type) {
  llvm::GlobalValue* var = dataVars_[data];
  if (!var) {
    llvm::StringRef stringRef(static_cast<const char*>(data), size);
    llvm::Constant* initializer = llvm::ConstantDataArray::getRaw(stringRef, size, byteType_);
    var = new llvm::GlobalVariable(*module_, initializer->getType(), true,
                                   llvm::GlobalVariable::InternalLinkage, initializer, "data");
    dataVars_[data] = var;
  }
  llvm::Value* controlBlock = CreateControlBlock(type);
  builder_->CreateStore(Int(size), GetArrayLengthAddress(controlBlock));
  llvm::Value* result = CreatePointer(var, controlBlock);
  // Add a ref so it can't actually be freed.
  RefStrongPtr(result);
  // But don't add an extra ref to the weak ptr count, so it *can* be freed.
  UnrefWeakPtr(result);
  return result;
}

Result CodeGenLLVM::Visit(Data* expr) {
  return GenerateGlobalData(expr->GetData(), expr->GetSize(), expr->GetType(types_));
}

Result CodeGenLLVM::Visit(Stmts* stmts) {
  for (auto p : stmts->GetVars()) {
    llvm::Function* f = builder_->GetInsertBlock()->getParent();
    CreateEntryBlockAlloca(f, p.get());
  }
  for (Stmt* const& i : stmts->GetStmts()) {
    i->Accept(this);
  }
  return nullptr;
}

void CodeGenLLVM::AppendTemporary(llvm::Value* value, Type* type) {
  temporaries_.push_back(ValueTypePair(value, type));
}

void CodeGenLLVM::DestroyTemporaries() {
  for (auto temporary : temporaries_) {
    if (temporary.type->IsStrongPtr()) {
      UnrefStrongPtr(temporary.value, static_cast<StrongPtrType*>(temporary.type));
    } else if (temporary.type->IsWeakPtr()) {
      UnrefWeakPtr(temporary.value);
    }
  }
  temporaries_.clear();
}

Result CodeGenLLVM::Visit(ExprStmt* stmt) {
  if (Expr* expr = stmt->GetExpr()) {
    llvm::Value* value = GenerateLLVM(expr);
    AppendTemporary(value, expr->GetType(types_));
  }
  DestroyTemporaries();
  return nullptr;
}

void CodeGenLLVM::Destroy(Type* type, llvm::Value* value) {
  if (type->IsClass()) {
    auto classType = static_cast<ClassType*>(type);
    auto destructor = classType->GetDestructor();
    assert(destructor);
    llvm::Function* function = GetOrCreateMethodStub(destructor);
    builder_->CreateCall(function->getFunctionType(), function, {value});
  } else if (type->IsArray()) {
    // FIXME: do array deletion
  } else if (type->IsRawPtr()) {
    auto temporary = scopedTemporaries_.find(value);
    if (temporary != scopedTemporaries_.end()) {
      type = temporary->second.type;
      value = temporary->second.value;
      if (type->IsStrongPtr()) {
        UnrefStrongPtr(value, static_cast<StrongPtrType*>(type));
      } else if (type->IsWeakPtr()) {
        UnrefWeakPtr(value);
      }
    }
  } else if (type->IsStrongPtr()) {
    value = builder_->CreateLoad(ConvertType(type), value);
    UnrefStrongPtr(value, static_cast<StrongPtrType*>(type));
  } else if (type->IsWeakPtr()) {
    value = builder_->CreateLoad(ConvertType(type), value);
    UnrefWeakPtr(value);
  }
}

Result CodeGenLLVM::Visit(DestroyStmt* node) {
  auto type = node->GetExpr()->GetType(types_);
  assert(type->IsRawPtr());
  type = static_cast<RawPtrType*>(type)->GetBaseType();
  auto value = GenerateLLVM(node->GetExpr());
  Destroy(type, value);
  return nullptr;
}

Result CodeGenLLVM::Visit(ReturnStatement* stmt) {
  llvm::Value* expr = stmt->GetExpr() ? GenerateLLVM(stmt->GetExpr()) : nullptr;
  DestroyTemporaries();
  builder_->CreateRet(expr);
  return nullptr;
}

Result CodeGenLLVM::Visit(WhileStatement* stmt) {
  Expr*             cond = stmt->GetCond();
  llvm::BasicBlock* topOfLoop = CreateBasicBlock("topOfLoop");
  llvm::BasicBlock* condition = cond ? CreateBasicBlock("condition") : nullptr;
  llvm::BasicBlock* after = CreateBasicBlock("after");
  builder_->CreateBr(condition ? condition : topOfLoop);
  builder_->SetInsertPoint(topOfLoop);
  Stmt* body = stmt->GetBody();
  if (body) body->Accept(this);
  if (cond) {
    builder_->CreateBr(condition);
    builder_->SetInsertPoint(condition);
    llvm::Value* v = GenerateLLVM(cond);
    builder_->CreateCondBr(v, topOfLoop, after);
  } else {
    builder_->CreateBr(topOfLoop);
  }
  builder_->SetInsertPoint(after);
  DestroyTemporaries();
  return nullptr;
}

Result CodeGenLLVM::Visit(DoStatement* stmt) {
  Stmt*             body = stmt->GetBody();
  Expr*             cond = stmt->GetCond();
  llvm::BasicBlock* topOfLoop = CreateBasicBlock("topOfLoop");
  llvm::BasicBlock* after = CreateBasicBlock("after");
  builder_->CreateBr(topOfLoop);
  builder_->SetInsertPoint(topOfLoop);
  if (body) body->Accept(this);
  if (cond) {
    llvm::Value* v = GenerateLLVM(cond);
    builder_->CreateCondBr(v, topOfLoop, after);
  } else {
    builder_->CreateBr(topOfLoop);
  }
  builder_->SetInsertPoint(after);
  DestroyTemporaries();
  return nullptr;
}

Result CodeGenLLVM::Visit(ForStatement* forStmt) {
  Stmt*           initStmt = forStmt->GetInitStmt();
  Expr*           cond = forStmt->GetCond();
  Stmt*           loopStmt = forStmt->GetLoopStmt();
  Stmt*           body = forStmt->GetBody();
  if (initStmt) initStmt->Accept(this);
  llvm::BasicBlock* topOfLoop = CreateBasicBlock("topOfLoop");
  llvm::BasicBlock* condition = cond ? CreateBasicBlock("forLoopCondition") : nullptr;
  llvm::BasicBlock* afterBlock = CreateBasicBlock("forLoopExit");
  builder_->CreateBr(cond ? condition : topOfLoop);
  builder_->SetInsertPoint(topOfLoop);
  if (body) body->Accept(this);
  if (loopStmt) loopStmt->Accept(this);
  if (cond) {
    builder_->CreateBr(condition);
    builder_->SetInsertPoint(condition);
    llvm::Value* v2 = GenerateLLVM(cond);
    builder_->CreateCondBr(v2, topOfLoop, afterBlock);
  } else {
    builder_->CreateBr(topOfLoop);
  }
  builder_->SetInsertPoint(afterBlock);
  DestroyTemporaries();
  return nullptr;
}

Result CodeGenLLVM::Visit(HeapAllocation* node) {
  Type*   type = node->GetType();
  int     qualifiers = 0;
  type = type->GetUnqualifiedType(&qualifiers);
  llvm::Type*  llvmType = ConvertType(type);
  llvm::Value* length = node->GetLength() ? GenerateLLVM(node->GetLength()) : nullptr;
  llvm::Value* value = CreateMalloc(llvmType, length);
  if (length) { value = CreatePointer(value, length); }
  return value;
}

Result CodeGenLLVM::Visit(BoolConstant* node) {
  return llvm::ConstantInt::get(boolType_, node->GetValue() ? 1 : 0, true);
}

Result CodeGenLLVM::Visit(FloatConstant* node) {
  return llvm::ConstantFP::get(floatType_, node->GetValue());
}

Result CodeGenLLVM::Visit(DoubleConstant* node) {
  return llvm::ConstantFP::get(doubleType_, node->GetValue());
}

Result CodeGenLLVM::Visit(IntConstant* node) {
  return llvm::ConstantInt::get(ConvertType(node->GetType(types_)), node->GetValue(), true);
}

Result CodeGenLLVM::Visit(UIntConstant* node) {
  return llvm::ConstantInt::get(ConvertType(node->GetType(types_)), node->GetValue(), true);
}

Result CodeGenLLVM::Visit(EnumConstant* node) {
  return llvm::ConstantInt::get(intType_, node->GetValue()->value, true);
}

Result CodeGenLLVM::Visit(VarExpr* expr) { return allocas_[expr->GetVar()]; }

Result CodeGenLLVM::Visit(TempVarExpr* node) {
  llvm::Type* type = ConvertType(node->GetType());
  auto*       tempVar = builder_->CreateAlloca(type);
  if (Expr* initExpr = node->GetInitExpr()) {
    builder_->CreateStore(GenerateLLVM(initExpr), tempVar);
  }
  return tempVar;
}

Result CodeGenLLVM::Visit(LoadExpr* expr) {
  llvm::Value* e = GenerateLLVM(expr->GetExpr());
  Type*        type = expr->GetType(types_);
  llvm::Value* r = builder_->CreateLoad(ConvertType(type), e);
  if (type->IsStrongPtr()) {
    RefStrongPtr(r);
  } else if (type->IsWeakPtr()) {
    RefWeakPtr(r);
  }
  return r;
}

Result CodeGenLLVM::Visit(ExprWithStmt* node) {
  llvm::Value* expr = GenerateLLVM(node->GetExpr());
  node->GetStmt()->Accept(this);
  return expr;
}

Result CodeGenLLVM::Visit(ZeroInitStmt* node) {
  llvm::Value* lhs = GenerateLLVM(node->GetLHS());
  Type*        type = node->GetLHS()->GetType(types_);
  assert(type->IsPtr());
  type = static_cast<PtrType*>(type)->GetBaseType();
  llvm::Value* zero = llvm::Constant::getNullValue(ConvertType(type));
  return builder_->CreateStore(zero, lhs);
}

Result CodeGenLLVM::Visit(StoreStmt* stmt) {
  llvm::Value* lhs = GenerateLLVM(stmt->GetLHS());
  int64_t size = stmt->GetRHS()->GetType(types_)->GetSizeInBytes();
  // If the RHS is a relatively large constant value, fold it into a static global in the
  // data segment and memcpy() from there.
  // FIXME: this should probably be done in a separate pass and produce a Data node.
  if (stmt->GetRHS()->IsConstant(types_) && size >= kMinAutoConstantSize) {
    char* data = new char[size];
    memset(data, 0, size);
    ConstantFolder constantFolder(types_, data);
    constantFolder.Resolve(stmt->GetRHS());
    llvm::StringRef stringRef(static_cast<const char*>(data), size);
    llvm::Constant* initializer = llvm::ConstantDataArray::getRaw(stringRef, size, byteType_);
    auto rhs = new llvm::GlobalVariable(*module_, initializer->getType(), true,
      llvm::GlobalVariable::InternalLinkage, initializer, "data");
    builder_->CreateMemCpy(lhs, {}, rhs, {}, size);
  } else {
    llvm::Value* rhs = GenerateLLVM(stmt->GetRHS());
    builder_->CreateStore(rhs, lhs);
  }
  if (stmt->GetRHS()->GetType(types_)->IsRawPtr() & !temporaries_.empty()) {
    auto temporary = temporaries_.back();
    temporaries_.pop_back();
    scopedTemporaries_[lhs] = temporary;
  }
  DestroyTemporaries();
  return {};
}

void CodeGenLLVM::CallSystemAbort() {
  llvm::Type* voidType = llvm::Type::getVoidTy(*context_);
  llvm::FunctionType* ft = llvm::FunctionType::get(voidType, false);
  llvm::FunctionCallee systemAbort = module_->getOrInsertFunction("System_Abort", ft);
  builder_->CreateCall(systemAbort, {});
}

void CodeGenLLVM::CreateBoundsCheck(llvm::Value* lhs, BinOpNode::Op op, llvm::Value* rhs) {
  llvm::Value*      condition = GenerateBinOpUInt(builder_, op, lhs, rhs);
  llvm::BasicBlock* outOfBounds = CreateBasicBlock("outOfBounds");
  llvm::BasicBlock* okBlock = CreateBasicBlock("ok");
  builder_->CreateCondBr(condition, outOfBounds, okBlock);
  builder_->SetInsertPoint(outOfBounds);
  CallSystemAbort();
  builder_->CreateBr(okBlock);
  builder_->SetInsertPoint(okBlock);
}

Result CodeGenLLVM::Visit(ArrayAccess* node) {
  llvm::Value* expr = GenerateLLVM(node->GetExpr());
  llvm::Value* index = GenerateLLVM(node->GetIndex());
  Type*        type = node->GetExpr()->GetType(types_);
  assert(type->IsRawPtr());
  type = static_cast<RawPtrType*>(type)->GetBaseType();
  assert(type->IsUnsizedArray());
  auto arrayType = static_cast<ArrayType*>(type);
  llvm::Type* llvmType = ConvertType(type);
  auto value = builder_->CreateExtractValue(expr, {0});
  auto length = builder_->CreateExtractValue(expr, {1});
  CreateBoundsCheck(index, BinOpNode::Op::GE, length);
  if (arrayType->GetElementPadding() > 0) {
    return builder_->CreateGEP(llvmType, value, {Int(0), index, Int(0)});
  } else {
    return builder_->CreateGEP(llvmType, value, {Int(0), index});
  }
}

Result CodeGenLLVM::Visit(SliceExpr* node) {
  auto expr = GenerateLLVM(node->GetExpr());

  auto type = node->GetExpr()->GetType(types_);
  assert(type->IsRawPtr());
  type = static_cast<RawPtrType*>(type)->GetBaseType();
  assert(type->IsUnsizedArray());

  auto ptr = builder_->CreateExtractValue(expr, {0});
  auto size = builder_->CreateExtractValue(expr, {1});

  auto start = node->GetStart() ? GenerateLLVM(node->GetStart()) : Int(0);
  auto end = node->GetEnd() ? GenerateLLVM(node->GetEnd()) : size;

  CreateBoundsCheck(start, BinOpNode::Op::GE, size);
  CreateBoundsCheck(end, BinOpNode::Op::GT, size);

  auto newPtr = builder_->CreateGEP(ConvertType(type), ptr, { Int(0), start });
  auto newSize = builder_->CreateSub(end, start, "sub");
  return CreatePointer(newPtr, newSize);
}

Result CodeGenLLVM::Visit(SmartToRawPtr* node) {
  llvm::Value* expr = GenerateLLVM(node->GetExpr());
  auto type = node->GetExpr()->GetType(types_);
  auto controlBlock = builder_->CreateExtractValue(expr, {1});
  llvm::BasicBlock* afterBlock = NullControlBlockCheck(controlBlock, BinOpNode::Op::EQ);
  llvm::BasicBlock* abortBlock = builder_->GetInsertBlock();
  CallSystemAbort();
  builder_->CreateBr(afterBlock);
  builder_->SetInsertPoint(afterBlock);
  if (type->IsWeakPtr()) {
    llvm::BasicBlock* afterRefCountCheck = CreateBasicBlock("afterRefCountCheck");
    auto strongRefCount = builder_->CreateLoad(intType_, GetStrongRefCountAddress(controlBlock));
    auto isZero = builder_->CreateICmpEQ(strongRefCount, Int(0));
    builder_->CreateCondBr(isZero, abortBlock, afterRefCountCheck);
    builder_->SetInsertPoint(afterRefCountCheck);
  }
  AppendTemporary(expr, type);
  auto value = builder_->CreateExtractValue(expr, {0});
  assert(type->IsStrongPtr() || type->IsWeakPtr());
  type = static_cast<PtrType*>(type)->GetBaseType();
  if (type->IsUnsizedArray() || type->IsUnsizedClass()) {
    auto controlBlock = builder_->CreateExtractValue(expr, {1});
    auto length = GetArrayLengthAddress(controlBlock);
    length = builder_->CreateLoad(intType_, length);
    return CreatePointer(value, length);
  }
  return value;
}

Result CodeGenLLVM::Visit(RawToSmartPtr* node) {
  llvm::Value* expr = GenerateLLVM(node->GetExpr());
  auto type = node->GetExpr()->GetType(types_);
  assert(type->IsRawPtr());
  type = static_cast<RawPtrType*>(type)->GetBaseType();
  auto controlBlock = CreateControlBlock(type);
  if (type->IsUnsizedArray() || type->IsUnsizedClass()) {
    auto length = builder_->CreateExtractValue(expr, {1});
    expr = builder_->CreateExtractValue(expr, {0});
    builder_->CreateStore(length, GetArrayLengthAddress(controlBlock));
  }
  return CreatePointer(expr, controlBlock);
}

Result CodeGenLLVM::Visit(ToRawArray* node) {
  llvm::Value* data = GenerateLLVM(node->GetData());
  llvm::Value* length = GenerateLLVM(node->GetLength());
  return CreatePointer(data, length);
}

Result CodeGenLLVM::Visit(FieldAccess* node) {
  llvm::Value* expr = GenerateLLVM(node->GetExpr());
  Type*        type = node->GetExpr()->GetType(types_);
  llvm::Value* length = nullptr;
  if (type->IsRawPtr()) {
    type = static_cast<RawPtrType*>(type)->GetBaseType();
    if (type->IsUnsizedClass()) {
      length = builder_->CreateExtractValue(expr, {1});
      expr = builder_->CreateExtractValue(expr, {0});
    }
  } else {
    llvm::AllocaInst* allocaInst = builder_->CreateAlloca(ConvertType(type));
    builder_->CreateStore(expr, allocaInst);
    expr = allocaInst;
  }
  Field* field = node->GetField();
  std::vector<llvm::Value*> indices = { Int(0), Int(field->index) };
  if (field->padding) indices.push_back(Int(0));
  auto result = builder_->CreateGEP(ConvertType(type), expr, indices);
  if (field->type->IsUnsizedArray()) { result = CreatePointer(result, length); }
  return result;
}

Result CodeGenLLVM::Visit(Initializer* node) {
  llvm::Type*  type = ConvertType(node->GetType());
  auto         args = node->GetArgList()->Get();
  llvm::Value* result = llvm::ConstantAggregateZero::get(type);
  if (node->GetType()->IsVector()) {
    int i = 0;
    for (auto arg : args) {
      llvm::Value* elt = GenerateLLVM(arg);
      llvm::Value* idx = llvm::ConstantInt::get(intType_, i++, true);
      result = builder_->CreateInsertElement(result, elt, idx);
    }
  } else if (node->GetType()->IsArray() || node->GetType()->IsMatrix()) {
    int i = 0;
    for (auto arg : args) {
      llvm::Value* v = GenerateLLVM(arg);
      result = builder_->CreateInsertValue(result, v, i++);
    }
  } else if (node->GetType()->IsClass()) {
    auto classType = static_cast<ClassType*>(node->GetType());
    assert(args.size() == classType->GetTotalFields());
    for (; classType != nullptr; classType = classType->GetParent()) {
      for (auto& field : classType->GetFields()) {
        auto arg = args[field->index];
        if (arg) {
          llvm::Value* v = GenerateLLVM(arg);
          if (field->padding) {
            auto type = PadType(ConvertType(field->type), field->padding);
            auto wrapper = llvm::ConstantAggregateZero::get(type);
            v = builder_->CreateInsertValue(wrapper, v, 0);
          }
          result = builder_->CreateInsertValue(result, v, field->index);
        }
      }
    }
  }
  return result;
}

Result CodeGenLLVM::Visit(LengthExpr* expr) {
  llvm::Value* value = GenerateLLVM(expr->GetExpr());
  Type*        type = expr->GetExpr()->GetType(types_);
  assert(type->IsRawPtr() && static_cast<RawPtrType*>(type)->GetBaseType()->IsUnsizedArray());
  return builder_->CreateExtractValue(value, {1});
}

Result CodeGenLLVM::Visit(ExtractElementExpr* expr) {
  llvm::Value* vec = GenerateLLVM(expr->GetExpr());
  llvm::Type*  type = ConvertType(expr->GetExpr()->GetType(types_));
  llvm::Value* index = llvm::ConstantInt::get(intType_, expr->GetIndex(), true);
  return builder_->CreateExtractElement(vec, index);
}

Result CodeGenLLVM::Visit(InsertElementExpr* expr) {
  llvm::Value* vec = GenerateLLVM(expr->GetExpr());
  llvm::Value* newElement = GenerateLLVM(expr->newElement());
  llvm::Type*  type = ConvertType(expr->GetExpr()->GetType(types_));
  llvm::Value* index = llvm::ConstantInt::get(intType_, expr->GetIndex(), true);
  return builder_->CreateInsertElement(vec, newElement, index);
}

Result CodeGenLLVM::Visit(SwizzleExpr* node) {
  llvm::Value* expr = GenerateLLVM(node->GetExpr());
  return builder_->CreateShuffleVector(expr, node->GetIndices());
}

Result CodeGenLLVM::Visit(IfStatement* ifStmt) {
  llvm::Value* v = GenerateLLVM(ifStmt->GetExpr());
  DestroyTemporaries();
  Stmt*             stmt = ifStmt->GetStmt();
  Stmt*             optElse = ifStmt->GetOptElse();
  llvm::BasicBlock* trueBlock = CreateBasicBlock("trueBlock");
  llvm::BasicBlock* elseBlock = optElse ? CreateBasicBlock("elseBlock") : nullptr;
  llvm::BasicBlock* afterBlock = CreateBasicBlock("afterBlock");
  builder_->CreateCondBr(v, trueBlock, elseBlock ? elseBlock : afterBlock);
  builder_->SetInsertPoint(trueBlock);
  if (stmt) stmt->Accept(this);
  if (!stmt || !stmt->ContainsReturn()) { builder_->CreateBr(afterBlock); }
  if (elseBlock) {
    builder_->SetInsertPoint(elseBlock);
    optElse->Accept(this);
    if (!optElse->ContainsReturn()) { builder_->CreateBr(afterBlock); }
  }
  builder_->SetInsertPoint(afterBlock);
  DestroyTemporaries();
  return nullptr;
}

llvm::Value* CodeGenLLVM::GenerateInlineAPIMethod(Method* method, ExprList* argList) {
  auto args = argList->Get();
  if (method->classType->GetName() == "Math") {
    if (method->name == "dot") {
      return GenerateDotProduct(GenerateLLVM(args[0]), GenerateLLVM(args[1]));
    } else if (method->name == "cross") {
      return GenerateCrossProduct(GenerateLLVM(args[0]), GenerateLLVM(args[1]));
    } else if (method->name == "length") {
      return GenerateVectorLength(GenerateLLVM(args[0]));
    } else if (method->name == "normalize") {
      return GenerateVectorNormalize(GenerateLLVM(args[0]));
    } else if (method->name == "transpose") {
      return GenerateTranspose(GenerateLLVM(args[0]), static_cast<MatrixType*>(args[0]->GetType(types_)));
    }
  }
  return nullptr;
}

llvm::Value* CodeGenLLVM::GenerateMethodCall(Method*             method,
                                             ExprList*           argList,
                                             Type*               returnType,
                                             const FileLocation& location) {
  if (auto result = GenerateInlineAPIMethod(method, argList)) { return result; }
  std::vector<llvm::Value*> args;
  llvm::Function*           function = GetOrCreateMethodStub(method);
  if (auto builtin = FindBuiltin(method)) { return std::invoke(builtin, this, location); }
  bool skipFirst = false;
  if (method->IsNative() && method->IsConstructor()) {
    skipFirst = true;
    if (method->classType->GetTemplate()) {
      auto allocation = argList->Get()[0];
      auto type = allocation->GetType(types_);
      assert(type->IsRawPtr());
      type = static_cast<RawPtrType*>(type)->GetBaseType();
      int qualifiers;
      type->GetUnqualifiedType(&qualifiers);
      // Prefix the args with the qualifiers
      args.push_back(llvm::ConstantInt::get(intType_, qualifiers));
    }
    for (Type* const& type : method->classType->GetTemplateArgs()) {
      args.push_back(CreateTypePtr(type));
    }
  }
  llvm::Intrinsic::ID intrinsic = function->getIntrinsicID();
  for (auto arg : argList->Get()) {
    if (skipFirst) { skipFirst = false; continue; }
    llvm::Value* v = GenerateLLVM(arg);
    Type*        type = arg->GetType(types_);
    AppendTemporary(v, type);
    if (method->IsNative() && !intrinsic) { v = ConvertToNative(type, v); }
    args.push_back(v);
  }
  if (intrinsic == llvm::Intrinsic::ctlz) {
    // is_zero_poison: false
    args.push_back(llvm::ConstantInt::get(boolType_, 0, true));
  }
  llvm::Value* result = builder_->CreateCall(function, args);
  if (method->IsNative() && !intrinsic) {
    result = ConvertFromNative(returnType, result);
  }
  return result;
}

Result CodeGenLLVM::Visit(MethodCall* node) {
  return GenerateMethodCall(node->GetMethod(), node->GetArgList(), node->GetMethod()->returnType,
                            node->GetFileLocation());
}

Result CodeGenLLVM::Visit(NullConstant* node) {
  std::vector<llvm::Type*> types;
  types.push_back(ptrType_); // Ptr
  types.push_back(ptrType_); // Control block
  llvm::Type* type = llvm::StructType::get(*context_, types);
  return llvm::ConstantAggregateZero::get(type);
}

Result CodeGenLLVM::Visit(UnaryOp* node) {
  llvm::Value* rhs = GenerateLLVM(node->GetRHS());
  Type*        type = node->GetRHS()->GetType(types_);
  llvm::Value* ret;
  if (node->GetOp() == UnaryOp::Op::Minus) {
    if (type->IsInteger() || type->IsIntegerVector()) {
      ret = builder_->CreateNeg(rhs, "neg");
    } else if (type->IsFloatingPoint() || type->IsFloatVector()) {
      ret = builder_->CreateFNeg(rhs, "fneg");
    } else {
      assert(false);
      ret = 0;
    }
  } else if (node->GetOp() == UnaryOp::Op::Negate) {
    llvm::Value* zero = llvm::ConstantInt::get(ConvertType(type), 0, true);
    return builder_->CreateICmpEQ(rhs, zero, "icmp");
  }
  return ret;
}

void CodeGenLLVM::ICE(ASTNode* node) {
  const FileLocation& location = node->GetFileLocation();
  fprintf(stderr,
          "%s:%d:  Internal compiler error:  CodeGenLLVM called on unresolved expression.\n",
          location.filename->c_str(), location.lineNum);
}

};  // namespace Toucan
