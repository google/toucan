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

#ifndef _CODEGEN_CODEGEN_SPIRV_H_
#define _CODEGEN_CODEGEN_SPIRV_H_

#include <list>
#include <unordered_map>

#include <ast/ast.h>
#include <utils/hash_pair.h>

namespace Toucan {

class SymbolTable;
using Code = std::vector<uint32_t>;

struct HashCode {
  std::size_t operator()(const Code& code) const {
    std::size_t r = 0;
    for (uint32_t v : code) {
      r ^= std::hash<uint32_t>()(v);
    }
    return r;
  }
};

using BindGroupList = std::vector<VarVector>;

class CodeGenSPIRV : public Visitor {
 public:
  CodeGenSPIRV(TypeTable* types) : types_(types) {}
  void     Run(Method* method);
  uint32_t GenerateSPIRV(ASTNode* node);
  void     GenCodeForMethod(Method* method, uint32_t functionId);
  uint32_t NextId() { return nextID_++; }
  void     Append(uint32_t opCode, const Code& args, Code* result);
  uint32_t Append(uint32_t opCode, uint32_t resultType, const Code& args, Code* result);
  uint32_t AppendTypeDecl(uint32_t opCode, const Code& args);
  void     AppendEntryPoint(uint32_t    executionModel,
                            uint32_t    entryPoint,
                            const char* name,
                            const Code& interface);
  void     AppendCode(uint32_t opCode, const Code& args);
  uint32_t AppendString(const char* str, Code* result);
  uint32_t AppendCode(uint32_t opCode, uint32_t resultType, const Code& args);
  uint32_t AppendDecl(uint32_t opCode, uint32_t resultType, const Code& args);
  uint32_t AppendExtInst(uint32_t extInst, uint32_t resultType, ExprList* argList);
  uint32_t DeclareVar(Var* var);
  void     DeclareBuiltInVars(Type* type, Code* interface);
  Type*    GetSampledType(ClassType* colorAttachment);
  uint32_t GetStorageClass(Type* type);
  uint32_t DeclareAndLoadInputVar(Type*      type,
                                  ShaderType shaderType,
                                  Code*      interface,
                                  uint32_t   location);
  uint32_t DeclareAndLoadInputVars(Type* type, ShaderType shaderType, Code* interface);
  void     DeclareAndStoreOutputVar(Type*      type,
                                    uint32_t   valueId,
                                    ShaderType shaderType,
                                    Code*      interface,
                                    uint32_t   location);
  void     DeclareAndStoreOutputVars(Type*      type,
                                     uint32_t   structId,
                                     ShaderType shaderType,
                                     Code*      interface);
  uint32_t AppendImageDecl(uint32_t dim, bool array, int qualifiers, const TypeList& templateArgs);
  uint32_t ConvertType(Type* type);
  uint32_t GetFunctionType(const Code& signature);
  uint32_t ConvertPointerToType(Type* type, uint32_t storageClass);
  uint32_t ConvertPointerToType(Type* type);
  uint32_t GetIntConstant(int32_t value);
  uint32_t GetFloatConstant(float value);
  uint32_t GetBoolConstant(bool value);
  uint32_t GetZeroConstant(Type* type);
  Result   Visit(ArgList* list) override;
  Result   Visit(ArrayAccess* node) override;
  Result   Visit(BinOpNode* node) override;
  Result   Visit(BoolConstant* expr) override;
  Result   Visit(CastExpr* expr) override;
  Result   Visit(SmartToRawPtr* node) override;
  Result   Visit(DoStatement* stmt) override;
  Result   Visit(ExprStmt* exprStmt) override;
  Result   Visit(ExprWithStmt* exprStmt) override;
  Result   Visit(FieldAccess* expr) override;
  Result   Visit(FloatConstant* expr) override;
  Result   Visit(ForStatement* stmt) override;
  Result   Visit(IfStatement* stmt) override;
  Result   Visit(Initializer* node) override;
  Result   Visit(IntConstant* intConstant) override;
  Result   Visit(LengthExpr* expr) override;
  Result   Visit(NewArrayExpr* expr) override;
  Result   Visit(NewExpr* expr) override;
  Result   Visit(NullConstant* expr) override;
  Result   Visit(ReturnStatement* stmt) override;
  Result   Visit(MethodCall* node) override;
  Result   Visit(Stmts* stmts) override;
  Result   Visit(InsertElementExpr* expr) override;
  Result   Visit(ExtractElementExpr* expr) override;
  Result   Visit(LoadExpr* expr) override;
  Result   Visit(RawToWeakPtr* node) override;
  Result   Visit(StoreStmt* stmt) override;
  Result   Visit(ZeroInitStmt* node) override;
  Result   Visit(UIntConstant* node) override;
  Result   Visit(UnaryOp* node) override;
  Result   Visit(VarExpr* expr) override;
  Result   Visit(WhileStatement* stmt) override;
  Result   Default(ASTNode* node) override {
    ICE(node);
    return nullptr;
  }
  void        ICE(ASTNode* node);
  const Code& header() const { return header_; }
  const Code& annotations() const { return annotations_; }
  const Code& decl() const { return decl_; }
  const Code& GetBody() const { return body_; }
  TypeTable*  types() const { return types_; }

 private:
  Type*    GetAndQualifyUnderlyingType(Type* type);
  void     ExtractPipelineVars(Method* entryPoint, Code* interface);
  void     ExtractPipelineVars(ClassType* classType,
                               ShaderType shaderType,
                               Code*      interface,
                               uint32_t*  inputCount,
                               uint32_t*  outputCount);
  uint32_t CreateVectorSplat(uint32_t value, VectorType* type);
  uint32_t CreateCast(Type* srcType, Type* dstType, uint32_t resultType, uint32_t valueId);
  uint32_t GetSampledImageType(Type* imageType);

  uint32_t                                     nextID_ = 1;
  uint32_t                                     glslStd450Import_;
  TypeTable*                                   types_;
  Code                                         header_;
  Code                                         annotations_;
  Code                                         decl_;
  Code                                         body_;
  std::unordered_map<Type*, uint32_t>          spirvTypes_;
  typedef std::pair<Type*, uint32_t>           PtrTypeKey;
  std::unordered_map<PtrTypeKey, uint32_t>     spirvPtrTypes_;
  std::unordered_map<Type*, uint32_t>          sampledImageTypes_;
  std::unordered_map<Code, uint32_t, HashCode> spirvFunctionTypes_;
  std::unordered_map<int32_t, uint32_t>        intConstants_;
  std::unordered_map<float, uint32_t>          floatConstants_;
  uint32_t                                     boolConstants_[2];
  std::unordered_map<Method*, uint32_t>        functions_;
  std::list<Method*>                           pendingMethods_;
  PtrType*                                     thisPtrType_ = nullptr;
  BindGroupList                                bindGroups_;
  std::vector<uint32_t>                        pipelineVars_;
  VarVector                                    builtInVars_;
};

};  // namespace Toucan
#endif
