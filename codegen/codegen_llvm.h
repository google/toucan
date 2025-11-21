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

#ifndef _CODEGEN_CODEGEN_LLVM_H_
#define _CODEGEN_CODEGEN_LLVM_H_

#include <unordered_map>

#include <llvm/IR/IRBuilder.h>

#include <ast/ast.h>

namespace llvm {
class Value;
class Type;
class PointerType;
class StructType;
class ConstantFolder;
class Constant;
class Function;
class GlobalVariable;
class Module;
namespace legacy {
class FunctionPassManager;
};
class LLVMContext;
};  // namespace llvm

namespace Toucan {

class CodeGenLLVM;

typedef llvm::IRBuilder<> LLVMBuilder;

struct ValueTypePair {
  ValueTypePair(llvm::Value* v, Type* t) : value(v), type(t) {}
  ValueTypePair() {}
  llvm::Value* value = nullptr;
  Type*        type = nullptr;
};

using DataVars = std::unordered_map<const void*, llvm::GlobalValue*>;
using DerefList = std::vector<ValueTypePair>;
using RefPtrTemporaries = std::unordered_map<llvm::Value*, ValueTypePair>;
using BuiltinCall = llvm::Value* (CodeGenLLVM::*)(const FileLocation& location);

class CodeGenLLVM : public Visitor {
 public:
  CodeGenLLVM(llvm::LLVMContext*                 context,
              TypeTable*                         types,
              llvm::Module*                      module,
              LLVMBuilder*                       builder,
              llvm::legacy::FunctionPassManager* fpm);
  void            Run(Stmts* stmts);
  llvm::Type*     ConvertType(Type* type);
  llvm::Type*     ConvertArrayElementType(ArrayType* type);
  llvm::Type*     ConvertTypeToNative(Type* type);
  llvm::Type*     PadType(llvm::Type* type, int padding);
  void            ConvertAndAppendFieldTypes(ClassType* classType, std::vector<llvm::Type*>* types);
  llvm::Type*     ControlBlockType();
  llvm::Constant* Int(int value);
  void            Push(llvm::Value* value);
  llvm::Value*    Pop();
  llvm::Value*    GenerateBinOp(BinOpNode* node, llvm::Value* lhs, llvm::Value* rhs, Type* type);
  llvm::Function* GetOrCreateMethodStub(Method* method);
  llvm::Value*    GetOrCreateDeleter(Type* type);
  void            GenCodeForMethod(Method* method);
  llvm::Value*    GetStrongRefCountAddress(llvm::Value* controlBlock);
  llvm::Value*    GetWeakRefCountAddress(llvm::Value* controlBlock);
  llvm::Value*    GetArrayLengthAddress(llvm::Value* controlBlock);
  llvm::Value*    GetClassTypeAddress(llvm::Value* controlBlock);
  llvm::Value*    GetDeleterAddress(llvm::Value* controlBlock);
  llvm::BasicBlock*     NullControlBlockCheck(llvm::Value* controlBlock, BinOpNode::Op op);
  void                  RefStrongPtr(llvm::Value* ptr);
  void                  UnrefStrongPtr(llvm::Value* ptr, StrongPtrType* type);
  void                  RefWeakPtr(llvm::Value* ptr);
  void                  UnrefWeakPtr(llvm::Value* ptr);
  llvm::Value*          ConvertToNative(Type* type, llvm::Value* value);
  llvm::Value*          ConvertFromNative(Type* type, llvm::Value* value);
  llvm::Intrinsic::ID   FindIntrinsic(Method* method);
  BuiltinCall           FindBuiltin(Method* method);
  llvm::Value*          GetSourceFile(const FileLocation& location);
  llvm::Value*          GetSourceLine(const FileLocation& location);
  void                  InitializeObject(llvm::Value* objPtr, ClassType* classType);
  llvm::AllocaInst*     CreateEntryBlockAlloca(llvm::Function* function, Var* var);
  llvm::Value*          CreatePointer(llvm::Value* obj, llvm::Value* controlBlockOrLength);
  llvm::Value*          CreateControlBlock(Type* type);
  llvm::Value*          CreateMalloc(llvm::Type* type, llvm::Value* arraySize);
  void                  CreateBoundsCheck(llvm::Value* lhs, BinOpNode::Op op, llvm::Value* rhs);
  llvm::Value*          GenerateLLVM(Expr* expr);
  llvm::Value*          GenerateDotProduct(llvm::Value* lhs, llvm::Value* rhs);
  llvm::Value*          GenerateCrossProduct(llvm::Value* lhs, llvm::Value* rhs);
  llvm::Value*          GenerateVectorLength(llvm::Value* value);
  llvm::Value*          GenerateVectorNormalize(llvm::Value* value);
  llvm::Value*          GenerateTranspose(llvm::Value* value, MatrixType* matrixType);
  llvm::Value*          GenerateMatrixMultiply(llvm::Value* lhs,
                                               llvm::Value* rhs,
                                               MatrixType*  lhsType,
                                               MatrixType*  rhsType);
  Result                Visit(ArrayAccess* expr) override;
  Result                Visit(BinOpNode* node) override;
  Result                Visit(BoolConstant* node) override;
  Result                Visit(CastExpr* expr) override;
  Result                Visit(Data* expr) override;
  Result                Visit(RawToSmartPtr* stmt) override;
  Result                Visit(SmartToRawPtr* stmt) override;
  Result                Visit(ToRawArray* node) override;
  Result                Visit(DestroyStmt* stmt) override;
  Result                Visit(DoStatement* stmt) override;
  Result                Visit(DoubleConstant* stmt) override;
  Result                Visit(EnumConstant* node) override;
  Result                Visit(ExprStmt* exprStmt) override;
  Result                Visit(ExprWithStmt* node) override;
  Result                Visit(ExtractElementExpr* expr) override;
  Result                Visit(FieldAccess* loadExpr) override;
  Result                Visit(FloatConstant* node) override;
  Result                Visit(ForStatement* forStmt) override;
  Result                Visit(HeapAllocation* node) override;
  Result                Visit(IfStatement* stmt) override;
  Result                Visit(Initializer* node) override;
  Result                Visit(InsertElementExpr* expr) override;
  Result                Visit(IntConstant* node) override;
  Result                Visit(LengthExpr* expr) override;
  Result                Visit(LoadExpr* expr) override;
  Result                Visit(NullConstant* node) override;
  Result                Visit(ReturnStatement* stmt) override;
  Result                Visit(MethodCall* node) override;
  Result                Visit(SliceExpr* expr) override;
  Result                Visit(Stmts* stmts) override;
  Result                Visit(SwizzleExpr* stmts) override;
  Result                Visit(TempVarExpr* expr) override;
  Result                Visit(VarExpr* expr) override;
  Result                Visit(StoreStmt* expr) override;
  Result                Visit(ZeroInitStmt* expr) override;
  Result                Visit(UIntConstant* expr) override;
  Result                Visit(UnaryOp* expr) override;
  Result                Visit(WhileStatement* stmt) override;
  Result                Default(ASTNode* node) override {
    ICE(node);
    return nullptr;
  }
  void               ICE(ASTNode* node);
  void               SetDebugOutput(bool debugOutput) { debugOutput_ = debugOutput; }
  llvm::GlobalValue* GetTypeList() const { return typeList_; }
  const std::vector<Type*>& GetReferencedTypes() { return referencedTypes_; }

 private:
  void         CallSystemAbort();
  llvm::Value* CreateCast(Type*        srcType,
                          Type*        dstType,
                          llvm::Value* value,
                          llvm::Type*  dstLLVMType);
  llvm::Value* GenerateInlineAPIMethod(Method* method, ExprList* argList);
  llvm::Value* GenerateMethodCall(Method*             method,
                                  ExprList*           args,
                                  Type*               returnType,
                                  const FileLocation& location);
  llvm::Value* GenerateGlobalData(const void* data, size_t size, Type* type);
  llvm::BasicBlock* CreateBasicBlock(const char* name);
  void         AppendTemporary(llvm::Value* value, Type* type);
  void         DestroyTemporaries();
  void         Destroy(Type* type, llvm::Value* value);
  llvm::Value* CreateTypePtr(Type* type);

 private:
  llvm::LLVMContext*                                    context_;
  TypeTable*                                            types_;
  llvm::Module*                                         module_;
  LLVMBuilder*                                          builder_;
  llvm::legacy::FunctionPassManager*                    fpm_;
  DataVars                                              dataVars_;
  llvm::Type*                                           intType_;
  llvm::Type*                                           floatType_;
  llvm::Type*                                           doubleType_;
  llvm::Type*                                           boolType_;
  llvm::Type*                                           byteType_;
  llvm::Type*                                           shortType_;
  llvm::PointerType*                                    ptrType_;
  llvm::FunctionType*                                   deleterType_;
  llvm::FunctionCallee                                  freeFunc_;
  llvm::Type*                                           controlBlockType_;
  bool                                                  debugOutput_;
  DerefList                                             temporaries_;
  RefPtrTemporaries                                     scopedTemporaries_;
  llvm::Type*                                           typeListType_;
  llvm::GlobalValue*                                    typeList_;
  std::unordered_map<Expr*, llvm::Value*>               exprCache_;
  std::unordered_map<Var*, llvm::AllocaInst*>           allocas_;
  std::unordered_map<Method*, llvm::Function*>          functions_;
  std::unordered_map<Type*, llvm::Function*>            deleters_;
  std::unordered_map<ClassType*, llvm::StructType*>     classPlaceholders_;
  std::vector<Type*>                                    referencedTypes_;
  std::unordered_map<Type*, llvm::Value*>               typeMap_;
};

};  // namespace Toucan
#endif
