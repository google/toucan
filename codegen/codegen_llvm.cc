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

#include <ast/symbol.h>
#include "codegen_spirv.h"

namespace Toucan {

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
  // This is used for function pointers, and void pointers.
  llvm::Type* voidType = llvm::Type::getVoidTy(*context_);
  funcPtrType_ = llvm::PointerType::get(llvm::FunctionType::get(voidType, false), 0);
  voidPtrType_ = llvm::PointerType::get(byteType_, 0);
  vtableType_ = llvm::PointerType::get(funcPtrType_, 0);
  controlBlockType_ = ControlBlockType();
  controlBlockPtrType_ = llvm::PointerType::get(controlBlockType_, 0);
  typeListType_ = llvm::PointerType::get(llvm::PointerType::get(voidPtrType_, 0), 0);
  typeList_ = new llvm::GlobalVariable(
      *module_, typeListType_, true, llvm::GlobalVariable::ExternalLinkage, nullptr, "_type_list");
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
      FillVTable(classType);
    }
  }
  stmts->Accept(this);
}

void CodeGenLLVM::ConvertAndAppendFieldTypes(ClassType*                classType,
                                             std::vector<llvm::Type*>* types) {
  if (classType->GetParent()) { ConvertAndAppendFieldTypes(classType->GetParent(), types); }
  for (const auto& field : classType->GetFields()) {
    field->paddedIndex = types->size();
    types->push_back(ConvertType(field->type));
    if (field->padding) { types->push_back(llvm::ArrayType::get(byteType_, field->padding)); }
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
  types.push_back(voidPtrType_);  // ClassType
  types.push_back(vtableType_);   // vtable
  return llvm::StructType::get(*context_, types);
}

llvm::Type* CodeGenLLVM::ConvertType(Type* type) {
  if (type->IsStrongPtr() || type->IsWeakPtr()) {
    std::vector<llvm::Type*> types;
    Type*                    baseType = static_cast<PtrType*>(type)->GetBaseType();
    baseType = baseType->GetUnqualifiedType();
    if (baseType->IsVoid() || baseType->IsFormalTemplateArg()) {
      types.push_back(voidPtrType_);
    } else {
      types.push_back(llvm::PointerType::get(ConvertType(baseType), 0));
    }
    types.push_back(llvm::PointerType::get(controlBlockType_, 0));
    return llvm::StructType::get(*context_, types);
  } else if (type->IsRawPtr()) {
    Type* baseType = static_cast<RawPtrType*>(type)->GetBaseType();
    baseType = baseType->GetUnqualifiedType();
    if (baseType->IsUnsizedArray() || baseType->IsUnsizedClass()) {
      std::vector<llvm::Type*> types;
      baseType = baseType->GetUnqualifiedType();
      types.push_back(llvm::PointerType::get(ConvertType(baseType), 0));
      types.push_back(intType_);
      return llvm::StructType::get(*context_, types);
    } else {
      return llvm::PointerType::get(ConvertType(baseType), 0);
    }
  } else if (type->IsArray()) {
    ArrayType* atype = static_cast<ArrayType*>(type);
    return llvm::ArrayType::get(ConvertArrayElementType(atype), atype->GetNumElements());
  } else if (type->IsClass()) {
    ClassType*               ctype = static_cast<ClassType*>(type);
    std::vector<llvm::Type*> types;
    ConvertAndAppendFieldTypes(ctype, &types);
    llvm::StructType* str = llvm::StructType::get(*context_, types);
    return str;
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
    llvm::Type* componentType = ConvertType(v->GetComponentType());
    return llvm::VectorType::get(componentType, v->GetLength(), false);
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
  if (int padding = arrayType->GetElementPadding()) {
    std::vector<llvm::Type*> types;
    types.push_back(result);
    types.push_back(llvm::ArrayType::get(byteType_, padding));
    result = llvm::StructType::get(*context_, types);
  }
  return result;
}

llvm::Type* CodeGenLLVM::ConvertTypeToNative(Type* type) {
  if (type->IsPtr()) {
    Type* baseType = static_cast<PtrType*>(type)->GetBaseType()->GetUnqualifiedType();
    if (baseType->IsClass() && static_cast<ClassType*>(baseType)->IsNative()) {
      // All pointers to native classes become ptr-to-base (obj pointer)
      return llvm::PointerType::get(ConvertType(baseType), 0);
    } else {
      // Pointers to anything else passed as Array* or Object*.
      return voidPtrType_;
    }
  } else if (type->IsVector()) {
    // All vectors must be passed as void*.
    return voidPtrType_;
  }
  return ConvertType(type);
}

llvm::Constant* CodeGenLLVM::Int(int value) { return llvm::ConstantInt::get(intType_, value); }

llvm::GlobalVariable* CodeGenLLVM::GetOrCreateVTable(ClassType* classType) {
  if (auto vtable = vtables_[classType]) return vtable;

  llvm::ArrayType*      arrayType = llvm::ArrayType::get(funcPtrType_, classType->GetVTableSize());
  llvm::GlobalVariable* vtable = new llvm::GlobalVariable(
      *module_, arrayType, true, llvm::GlobalVariable::ExternalLinkage, nullptr, "vtable");
  return vtables_[classType] = vtable;
}

void CodeGenLLVM::FillVTable(ClassType* classType) {
  llvm::GlobalVariable* vtable = GetOrCreateVTable(classType);
  llvm::ArrayType*      arrayType = llvm::ArrayType::get(funcPtrType_, classType->GetVTableSize());
  std::vector<llvm::Constant*> functions;
  for (Method* const& method : classType->GetVTable()) {
    llvm::Function* function = GetOrCreateMethodStub(method);
    llvm::Constant* voidFunc = llvm::ConstantExpr::getBitCast(function, funcPtrType_);
    functions.push_back(voidFunc);
  }
  llvm::Constant* initializer = llvm::ConstantArray::get(arrayType, functions);
  vtable->setInitializer(initializer);
}

llvm::Value* CodeGenLLVM::CreateTypePtr(Type* type) {
  llvm::Value* ptr = builder_->CreateLoad(typeListType_, typeList_);
  ptr = builder_->CreateGEP(typeListType_, ptr, {Int(types_->GetTypeID(type))});
  return builder_->CreateLoad(voidPtrType_, ptr);
}

llvm::Value* CodeGenLLVM::CreateControlBlock(Type* type) {
  llvm::Value* controlBlock = CreateMalloc(controlBlockType_, 0);
  builder_->CreateStore(Int(1), GetStrongRefCountAddress(controlBlock));
  builder_->CreateStore(Int(1), GetWeakRefCountAddress(controlBlock));
  int arrayLength = type->IsArray() ? static_cast<ArrayType*>(type)->GetNumElements() : 0;
  builder_->CreateStore(Int(arrayLength), GetArrayLengthAddress(controlBlock));
  builder_->CreateStore(CreateTypePtr(type), GetClassTypeAddress(controlBlock));
  type = type->GetUnqualifiedType();
  if (type->IsClass()) {
    llvm::GlobalVariable* vtable = GetOrCreateVTable(static_cast<ClassType*>(type));
    llvm::Value*          indices[] = {Int(0), Int(0)};
    llvm::Type*           type = vtable->getValueType();
    llvm::Value*          vtableValue = llvm::ConstantExpr::getGetElementPtr(type, vtable, indices);
    builder_->CreateStore(vtableValue, GetVTableAddress(controlBlock));
  }
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

llvm::Value* CodeGenLLVM::GetVTableAddress(llvm::Value* controlBlock) {
  return builder_->CreateGEP(controlBlockType_, controlBlock, {Int(0), Int(4)});
}

llvm::BasicBlock* CodeGenLLVM::NullControlBlockCheck(llvm::Value* controlBlock) {
  llvm::Value*      nullControlBlock = llvm::ConstantPointerNull::get(controlBlockPtrType_);
  llvm::Value*      nonNull = builder_->CreateICmpNE(controlBlock, nullControlBlock);
  llvm::Function*   f = builder_->GetInsertBlock()->getParent();
  llvm::BasicBlock* nonNullBlock = llvm::BasicBlock::Create(*context_, "nonNull", f);
  llvm::BasicBlock* afterBlock = llvm::BasicBlock::Create(*context_, "after", f);
  builder_->CreateCondBr(nonNull, nonNullBlock, afterBlock);
  builder_->SetInsertPoint(nonNullBlock);
  return afterBlock;
}

void CodeGenLLVM::RefStrongPtr(llvm::Value* ptr) {
  llvm::Value*      controlBlock = builder_->CreateExtractValue(ptr, {1});
  llvm::BasicBlock* afterBlock = NullControlBlockCheck(controlBlock);

  llvm::Value* address = GetStrongRefCountAddress(controlBlock);
  llvm::Value* refCount = builder_->CreateLoad(intType_, address);
  refCount = builder_->CreateAdd(refCount, Int(1));
  builder_->CreateStore(refCount, address);

  builder_->CreateBr(afterBlock);
  builder_->SetInsertPoint(afterBlock);
  RefWeakPtr(ptr);
}

void CodeGenLLVM::UnrefStrongPtr(llvm::Value* ptr, StrongPtrType* type) {
  llvm::Value*      controlBlock = builder_->CreateExtractValue(ptr, {1});
  llvm::BasicBlock* afterBlock = NullControlBlockCheck(controlBlock);

  llvm::Value* address = GetStrongRefCountAddress(controlBlock);
  llvm::Value* refCount = builder_->CreateLoad(intType_, address);
  refCount = builder_->CreateSub(refCount, Int(1));
  builder_->CreateStore(refCount, address);
  llvm::Value*      isZero = builder_->CreateICmpEQ(refCount, Int(0));
  llvm::Function*   f = builder_->GetInsertBlock()->getParent();
  llvm::BasicBlock* trueBlock = llvm::BasicBlock::Create(*context_, "trueBlock", f);
  builder_->CreateCondBr(isZero, trueBlock, afterBlock);
  builder_->SetInsertPoint(trueBlock);
  bool  isNativeClass = false;
  Type* baseType = type->GetBaseType()->GetUnqualifiedType();
  if (baseType->IsClass()) {
    auto            classType = static_cast<ClassType*>(baseType);
    Method*         destructor = classType->GetVTable()[0];
    llvm::Function* function = GetOrCreateMethodStub(destructor);
    llvm::Value*    v = GetVTableAddress(controlBlock);
    llvm::Value*    vtable = builder_->CreateLoad(vtableType_, v);
    llvm::Value*    gep = builder_->CreateGEP(funcPtrType_, vtable, Int(0));
    llvm::Value*    func = builder_->CreateLoad(funcPtrType_, gep);
    llvm::Value*    typedFunc =
        builder_->CreateBitCast(func, llvm::PointerType::get(function->getFunctionType(), 0));
    llvm::Value* arg = builder_->CreateExtractValue(ptr, {0});
    if (classType->IsUnsizedClass()) {
      auto length = builder_->CreateLoad(intType_, GetArrayLengthAddress(controlBlock));
      arg = CreatePointer(arg, length);
    }
    isNativeClass = classType->IsNative();
    builder_->CreateCall(function->getFunctionType(), typedFunc, {arg});
  }
  if (!isNativeClass) { GenerateFree(builder_->CreateExtractValue(ptr, {0})); }
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
  llvm::BasicBlock* afterBlock = NullControlBlockCheck(controlBlock);

  llvm::Value* address = GetWeakRefCountAddress(controlBlock);
  llvm::Value* refCount = builder_->CreateLoad(intType_, address);
  refCount = builder_->CreateAdd(refCount, Int(1));
  builder_->CreateStore(refCount, address);

  builder_->CreateBr(afterBlock);
  builder_->SetInsertPoint(afterBlock);
}

void CodeGenLLVM::UnrefWeakPtr(llvm::Value* ptr) {
  llvm::Value*      controlBlock = builder_->CreateExtractValue(ptr, {1});
  llvm::BasicBlock* afterBlock = NullControlBlockCheck(controlBlock);

  llvm::Value* address = GetWeakRefCountAddress(controlBlock);
  llvm::Value* refCount = builder_->CreateLoad(intType_, address);
  refCount = builder_->CreateSub(refCount, Int(1));
  builder_->CreateStore(refCount, address);
  llvm::Value*      isZero = builder_->CreateICmpEQ(refCount, Int(0));
  llvm::Function*   f = builder_->GetInsertBlock()->getParent();
  llvm::BasicBlock* trueBlock = llvm::BasicBlock::Create(*context_, "trueBlock", f);
  builder_->CreateCondBr(isZero, trueBlock, afterBlock);
  builder_->SetInsertPoint(trueBlock);
  GenerateFree(controlBlock);

  builder_->CreateBr(afterBlock);
  builder_->SetInsertPoint(afterBlock);
}

llvm::Function* CodeGenLLVM::GetOrCreateMethodStub(Method* method) {
  if (auto function = functions_[method]) { return function; }
  std::vector<llvm::Type*> params;
  llvm::Intrinsic::ID      intrinsic = llvm::Intrinsic::not_intrinsic;
  bool skipFirst = false;
  if (method->classType->IsNative()) {
    if (method->templateMethod) { return GetOrCreateMethodStub(method->templateMethod); }
    if (method->IsConstructor()) {
      skipFirst = true;
      if (method->classType->IsClassTemplate()) {
        // First argument is the storage qualifier (as uint)
        params.push_back(intType_);
        ClassTemplate* classTemplate = static_cast<ClassTemplate*>(method->classType);
        for (Type* const& type : classTemplate->GetFormalTemplateArgs()) {
          params.push_back(voidPtrType_);
        }
      }
    }
    intrinsic = FindIntrinsic(method);
  }
  bool nativeTypes = method->classType->IsNative() && !intrinsic;
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
    function = llvm::Intrinsic::getDeclaration(module_, intrinsic, params);
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

  function->setCallingConv(llvm::CallingConv::C);
  return functions_[method] = function;
}

llvm::Intrinsic::ID CodeGenLLVM::FindIntrinsic(Method* method) {
  constexpr struct {
    const char*         className;
    const char*         methodName;
    llvm::Intrinsic::ID id;
  } intrinsics[] = {
      "Math", "sqrt", llvm::Intrinsic::sqrt, "Math", "sin",  llvm::Intrinsic::sin,
      "Math", "cos",  llvm::Intrinsic::cos,  "Math", "fabs", llvm::Intrinsic::fabs,
      "Math", "clz",  llvm::Intrinsic::ctlz,
  };

  for (auto intrinsic : intrinsics) {
    if (method->name == intrinsic.methodName &&
        method->classType->GetName() == intrinsic.className) {
      return intrinsic.id;
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
  if (method->classType->IsNative()) return;
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
  if (method->stmts) { method->stmts->Accept(this); }
  verifyFunction(*function);
  fpm_->run(*function);
  builder_->SetInsertPoint(whereWasI);
#if !defined(NDEBUG)
//  if (debugOutput_) function->dump();
#endif
}

llvm::Value* CodeGenLLVM::CreateMalloc(llvm::Type* type, llvm::Value* arraySize) {
  llvm::PointerType* ptrType = llvm::PointerType::get(type, 0);
  // TODO(senorblanco):  initialize this once, not every time
  std::vector<llvm::Type*> args;
  args.push_back(intType_);
#if TARGET_OS_IS_WIN && TARGET_CPU_IS_X86
  args.push_back(intType_);
#endif
  llvm::FunctionType* ft = llvm::FunctionType::get(voidPtrType_, args, false);
  llvm::Value*        indices[] = {arraySize ? arraySize : llvm::ConstantInt::get(intType_, 1)};
  llvm::Value*        nullPtr = llvm::ConstantPointerNull::get(ptrType);
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
  return builder_->CreateBitCast(ptr, ptrType);
}

void CodeGenLLVM::GenerateFree(llvm::Value* value) {
  std::vector<llvm::Type*> args;
  args.push_back(voidPtrType_);
  llvm::Type*         voidType = llvm::Type::getVoidTy(*context_);
  llvm::FunctionType* ft = llvm::FunctionType::get(voidType, args, false);
#if TARGET_OS_IS_WIN && TARGET_CPU_IS_X86
  llvm::FunctionCallee free = module_->getOrInsertFunction("_aligned_free", ft);
#else
  llvm::FunctionCallee free = module_->getOrInsertFunction("free", ft);
#endif
  value = builder_->CreateBitCast(value, voidPtrType_);
  builder_->CreateCall(free, value);
}

llvm::Value* CodeGenLLVM::GenerateLLVM(Expr* expr) {
  if (auto result = exprCache_[expr]) { return result; }
  auto value = static_cast<llvm::Value*>(std::get<void*>(expr->Accept(this)));
  return exprCache_[expr] = value;
}

llvm::Value* CodeGenLLVM::ConvertToNative(Type* type, llvm::Value* value) {
  if (type->IsRawPtr() && static_cast<RawPtrType*>(type)->GetBaseType()->IsUnsizedArray()) {
    // Allocate a stack var for Array, then pass a pointer to that (Array*).
    llvm::Value* alloc = builder_->CreateAlloca(ConvertType(type), 0, "Array");
    builder_->CreateStore(value, alloc);
    value = alloc;
  } else if (type->IsStrongPtr() || type->IsWeakPtr()) {
    Type* baseType = static_cast<PtrType*>(type)->GetBaseType();
    baseType = baseType->GetUnqualifiedType();
    if (baseType && baseType->IsClass() && static_cast<ClassType*>(baseType)->IsNative()) {
      value = builder_->CreateExtractValue(value, {0});
    } else {
      // Allocate a stack var for Object, then pass a pointer to that (Object*).
      llvm::Value* alloc = builder_->CreateAlloca(ConvertType(type), 0, "Object");
      builder_->CreateStore(value, alloc);
      value = alloc;
    }
  } else if (type->IsVector()) {
    llvm::Value* alloc = builder_->CreateAlloca(ConvertType(type));
    builder_->CreateStore(value, alloc);
    value = builder_->CreateBitCast(alloc, voidPtrType_);
  }
  return value;
}

llvm::Value* CodeGenLLVM::ConvertFromNative(Type* type, llvm::Value* value) {
  if (type->IsStrongPtr() || type->IsWeakPtr()) {
    Type* baseType = static_cast<PtrType*>(type)->GetBaseType();
    Type* unqualifiedType = baseType->GetUnqualifiedType();
    if (unqualifiedType->IsClass() && static_cast<ClassType*>(unqualifiedType)->IsNative()) {
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

static llvm::Value* GenerateBinOpInt(LLVMBuilder* builder,
                                     BinOpNode*   node,
                                     llvm::Value* lhs,
                                     llvm::Value* rhs) {
  switch (node->GetOp()) {
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

static llvm::Value* GenerateBinOpUInt(LLVMBuilder* builder,
                                      BinOpNode*   node,
                                      llvm::Value* lhs,
                                      llvm::Value* rhs) {
  switch (node->GetOp()) {
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

static llvm::Value* GenerateBinOpFloat(LLVMBuilder* builder,
                                       BinOpNode*   node,
                                       llvm::Value* lhs,
                                       llvm::Value* rhs) {
  switch (node->GetOp()) {
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

llvm::Value* CodeGenLLVM::GenerateTranspose(llvm::Value* srcMatrix, MatrixType* srcMatrixType) {
  VectorType* srcColumnType = srcMatrixType->GetColumnType();
  unsigned    numSrcColumns = srcMatrixType->GetNumColumns();
  unsigned    numSrcRows = srcColumnType->GetLength();

  VectorType* dstColumnType = types_->GetVector(srcColumnType->GetComponentType(), numSrcColumns);
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

llvm::Value* CodeGenLLVM::GenerateDotProduct(llvm::Value* lhs, llvm::Value* rhs, VectorType* type) {
  llvm::Value* product = builder_->CreateFMul(lhs, rhs);
  llvm::Value* sum = builder_->CreateExtractElement(product, Int(0));
  for (int i = 1; i < type->GetLength(); ++i) {
    llvm::Value* value = builder_->CreateExtractElement(product, Int(i));
    sum = builder_->CreateFAdd(sum, value);
  }
  return sum;
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
  unsigned numRows = columnType->GetLength();

  std::vector<llvm::Value*> lhsRows(numRows);
  for (unsigned row = 0; row < numRows; ++row) {
    lhsRows[row] = builder_->CreateExtractValue(lhsT, row);
  }
  llvm::Value* dstMatrix = llvm::ConstantAggregateZero::get(matrixTypeLLVM);
  for (unsigned col = 0; col < numColumns; ++col) {
    llvm::Value* rhsCol = builder_->CreateExtractValue(rhs, col);
    llvm::Value* dstColumn = llvm::ConstantAggregateZero::get(columnTypeLLVM);
    for (unsigned row = 0; row < numRows; ++row) {
      llvm::Value* value = GenerateDotProduct(lhsRows[row], rhsCol, columnType);
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
      return GenerateBinOpUInt(builder_, node, lhs, rhs);
    } else {
      return GenerateBinOpInt(builder_, node, lhs, rhs);
    }
  } else if (type->IsFloatingPoint() || type->IsFloatVector()) {
    return GenerateBinOpFloat(builder_, node, lhs, rhs);
  } else if (type->IsMatrix() && node->GetOp() == BinOpNode::MUL) {
    auto matrixType = static_cast<MatrixType*>(type);
    return GenerateMatrixMultiply(lhs, rhs, matrixType, matrixType);
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
    if (lhsVectorType->GetComponentType() == rhsType) {
      rhs = builder_->CreateVectorSplat(lhsVectorType->GetLength(), rhs);
      ret = GenerateBinOp(node, lhs, rhs, lhsType);
    } else {
      assert(false);
    }
  } else if (TypeTable::ScalarVector(lhsType, rhsType)) {
    VectorType* rhsVectorType = static_cast<VectorType*>(rhsType);
    if (rhsVectorType->GetComponentType() == lhsType) {
      lhs = builder_->CreateVectorSplat(rhsVectorType->GetLength(), lhs);
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
    for (int i = 0; i < vectorType->GetLength(); i += 1) {
      llvm::Value* row = builder_->CreateExtractValue(lhsT, i);
      llvm::Value* value = GenerateDotProduct(row, rhs, vectorType);
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
    return CreateCast(static_cast<VectorType*>(srcType)->GetComponentType(),
                      static_cast<VectorType*>(dstType)->GetComponentType(), value,
                      ConvertType(dstType));
  } else if (srcType->IsPtr() && dstType->IsPtr()) {
    if (srcType->IsStrongPtr() && dstType->IsWeakPtr()) {
      RefWeakPtr(value);
      AppendTemporary(value, static_cast<StrongPtrType*>(srcType));
    }
    Type*        dstBase = static_cast<PtrType*>(dstType)->GetBaseType();
    llvm::Value* ptr = builder_->CreateExtractValue(value, {0});
    llvm::Value* controlBlock = builder_->CreateExtractValue(value, {1});
    llvm::Type*  newPtrType =
        dstBase->IsVoid() ? voidPtrType_ : llvm::PointerType::get(ConvertType(dstBase), 0);
    llvm::Value* newPtr = builder_->CreateBitCast(ptr, newPtrType);
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

Result CodeGenLLVM::Visit(DestroyStmt* node) {
  auto type = node->GetExpr()->GetType(types_);
  assert(type->IsRawPtr());
  type = static_cast<RawPtrType*>(type)->GetBaseType();
  auto value = GenerateLLVM(node->GetExpr());
  if (type->IsRawPtr()) {
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
  llvm::Function*   f = builder_->GetInsertBlock()->getParent();
  llvm::BasicBlock* topOfLoop = llvm::BasicBlock::Create(*context_, "topOfLoop", f);
  llvm::BasicBlock* condition = cond ? llvm::BasicBlock::Create(*context_, "condition", f) : 0;
  llvm::BasicBlock* after = llvm::BasicBlock::Create(*context_, "after", f);
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
  llvm::Function*   f = builder_->GetInsertBlock()->getParent();
  llvm::BasicBlock* topOfLoop = llvm::BasicBlock::Create(*context_, "topOfLoop", f);
  llvm::BasicBlock* after = llvm::BasicBlock::Create(*context_, "after", f);
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
  llvm::Function* f = builder_->GetInsertBlock()->getParent();
  if (initStmt) initStmt->Accept(this);
  llvm::BasicBlock* topOfLoop = llvm::BasicBlock::Create(*context_, "topOfLoop", f);
  llvm::BasicBlock* condition =
      cond ? llvm::BasicBlock::Create(*context_, "forLoopCondition", f) : 0;
  llvm::BasicBlock* afterBlock = llvm::BasicBlock::Create(*context_, "forLoopExit", f);
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
  llvm::Value* rhs = GenerateLLVM(stmt->GetRHS());
  llvm::Value* lhs = GenerateLLVM(stmt->GetLHS());
  if (stmt->GetRHS()->GetType(types_)->IsRawPtr() & !temporaries_.empty()) {
    auto temporary = temporaries_.back();
    temporaries_.pop_back();
    scopedTemporaries_[lhs] = temporary;
  }
  DestroyTemporaries();
  return builder_->CreateStore(rhs, lhs);
}

Result CodeGenLLVM::Visit(ArrayAccess* node) {
  llvm::Value* expr = GenerateLLVM(node->GetExpr());
  llvm::Value* index = GenerateLLVM(node->GetIndex());
  Type*        type = node->GetExpr()->GetType(types_);
  assert(type->IsRawPtr());
  type = static_cast<RawPtrType*>(type)->GetBaseType();
  assert(type->IsUnsizedArray());
  llvm::Type* llvmType = ConvertType(type);
  expr = builder_->CreateExtractValue(expr, {0});
  // FIXME: do bounds checking here
  if (type->IsArray() && static_cast<ArrayType*>(type)->GetElementPadding() > 0) {
    return builder_->CreateGEP(llvmType, expr, {Int(0), index, Int(0)});
  } else {
    return builder_->CreateGEP(llvmType, expr, {Int(0), index});
  }
}

Result CodeGenLLVM::Visit(SmartToRawPtr* node) {
  llvm::Value* expr = GenerateLLVM(node->GetExpr());
  auto type = node->GetExpr()->GetType(types_);
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
  auto result = builder_->CreateGEP(ConvertType(type), expr, {Int(0), Int(field->paddedIndex)});
  if (field->type->IsUnsizedArray()) { result = CreatePointer(result, length); }
  return result;
}

Result CodeGenLLVM::Visit(Initializer* node) {
  llvm::Type*  type = ConvertType(node->GetType());
  llvm::Value* result = llvm::ConstantAggregateZero::get(type);
  if (node->GetType()->IsVector()) {
    int i = 0;
    for (auto arg : node->GetArgList()->Get()) {
      llvm::Value* elt = GenerateLLVM(arg);
      llvm::Value* idx = llvm::ConstantInt::get(intType_, i++, true);
      result = builder_->CreateInsertElement(result, elt, idx);
    }
  } else if (node->GetType()->IsArray() || node->GetType()->IsMatrix()) {
    int i = 0;
    for (auto arg : node->GetArgList()->Get()) {
      llvm::Value* v = GenerateLLVM(arg);
      result = builder_->CreateInsertValue(result, v, i++);
    }
  } else if (node->GetType()->IsClass()) {
    auto classType = static_cast<ClassType*>(node->GetType());
    auto args = node->GetArgList()->Get();
    for (; classType != nullptr; classType = classType->GetParent()) {
      for (auto& field : classType->GetFields()) {
        if (field->index < args.size()) {
          auto arg = args[field->index];
          if (arg) {
            llvm::Value* v = GenerateLLVM(arg);
            result = builder_->CreateInsertValue(result, v, field->paddedIndex);
            AppendTemporary(v, arg->GetType(types_));
          }
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

Result CodeGenLLVM::Visit(IfStatement* ifStmt) {
  llvm::Value* v = GenerateLLVM(ifStmt->GetExpr());
  DestroyTemporaries();
  Stmt*             stmt = ifStmt->GetStmt();
  Stmt*             optElse = ifStmt->GetOptElse();
  llvm::Function*   f = builder_->GetInsertBlock()->getParent();
  llvm::BasicBlock* trueBlock = llvm::BasicBlock::Create(*context_, "trueBlock", f);
  llvm::BasicBlock* elseBlock = optElse ? llvm::BasicBlock::Create(*context_, "elseBlock", f) : 0;
  llvm::BasicBlock* afterBlock = llvm::BasicBlock::Create(*context_, "afterBlock", f);
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

llvm::Value* CodeGenLLVM::GenerateMethodCall(Method*             method,
                                             ExprList*           argList,
                                             Type*               returnType,
                                             const FileLocation& location) {
  std::vector<llvm::Value*> args;
  llvm::Function*           function = GetOrCreateMethodStub(method);
  if (auto builtin = FindBuiltin(method)) { return std::invoke(builtin, this, location); }
  bool skipFirst = false;
  if (method->classType->IsNative() && method->IsConstructor()) {
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
    if (method->classType->IsNative() && !intrinsic) { v = ConvertToNative(type, v); }
    args.push_back(v);
  }
  if (intrinsic == llvm::Intrinsic::ctlz) {
    // is_zero_poison: false
    args.push_back(llvm::ConstantInt::get(boolType_, 0, true));
  }
  llvm::Value* result;
  if (method->modifiers & Method::Modifier::Virtual) {
    llvm::Value* objPtr = *args.begin();
    int          index = method->index;
    llvm::Value* controlBlock = builder_->CreateExtractValue(objPtr, {1});
    llvm::Value* v = GetVTableAddress(controlBlock);
    llvm::Value* vtable = builder_->CreateLoad(vtableType_, v, "vtable");
    llvm::Value* indices[] = {llvm::ConstantInt::get(intType_, index)};
    llvm::Value* gep = builder_->CreateGEP(funcPtrType_, vtable, indices);
    llvm::Value* func = builder_->CreateLoad(funcPtrType_, gep, "func");
    llvm::Value* typedFunc =
        builder_->CreateBitCast(func, llvm::PointerType::get(function->getFunctionType(), 0));
    result = builder_->CreateCall(function->getFunctionType(), typedFunc, args);
  } else {
    result = builder_->CreateCall(function, args);
  }
  if (method->classType->IsNative() && !intrinsic) {
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
  types.push_back(voidPtrType_);
  types.push_back(controlBlockPtrType_);
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
