// Copyright 2024 The Toucan Authors
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

#ifndef _AST_AST_SPIRV_PREP_PASS_H_
#define _AST_AST_SPIRV_PREP_PASS_H_

#include "copy_visitor.h"

#include <unordered_set>

namespace Toucan {

using VarAliasMap = std::unordered_map<Var*, Expr*>;
using UnfoldedVarMap = std::unordered_map<Var*, VarVector>;
using UnfoldedPtrMap = std::unordered_map<Var*, std::shared_ptr<Var>>;
using WrapperMap = std::unordered_map<Type*, Type*>;
using WrapperSet = std::unordered_set<Type*>;

struct MethodKey {
  Method*           method;
  std::vector<Var*> globalArgs;
  bool operator==(const MethodKey& other) const {
    return method == other.method && globalArgs == other.globalArgs;
  }

  struct Hash {
    std::size_t operator()(const MethodKey& key) const {
      std::size_t r = std::hash<void*>()(key.method);
      for (auto globalArg : key.globalArgs) {
        r ^= std::hash<void*>()(globalArg);
      }
      return r;
    }
  };
};

using MethodMap = std::unordered_map<MethodKey, std::unique_ptr<Method>, MethodKey::Hash>;

using BindGroupList = std::vector<VarVector>;

class ShaderPrepPass : public CopyVisitor {
 public:
  ShaderPrepPass(NodeVector* nodes, TypeTable* types);
  Method*              Run(Method* entryPoint);
  Result               Visit(CastExpr* node) override;
  Result               Visit(FieldAccess* node) override;
  Result               Visit(LoadExpr* node) override;
  Result               Visit(MethodCall* node) override;
  Result               Visit(SmartToRawPtr* node) override;
  Result               Visit(Stmts* node) override;
  Result               Visit(StoreStmt* node) override;
  Result               Visit(ToRawArray* node) override;
  Result               Visit(VarExpr* node) override;
  Result               Visit(ZeroInitStmt* node) override;
  Result               Default(ASTNode* node) override;
  const VarVector&     GetInputs() const { return inputs_; }
  const VarVector&     GetOutputs() const { return outputs_; }
  const BindGroupList& GetBindGroups() const { return bindGroups_; }
  const VarVector&     GetBuiltInVars() const { return builtInVars_; }

 private:
  Expr*   ResolveVar(Var* var);
  bool    TypeIsValidForShaderType(Type*) const;
  Type*   ConvertType(Type* type);
  Result  ResolveNativeMethodCall(MethodCall* node);
  Method* PrepMethod(Method* method, std::vector<Var*> globalArgs);
  void    UnfoldClass(ClassType* classType, VarVector* vars, VarVector* localVars, VarVector* globalVars);
  void    UnfoldVar(std::shared_ptr<Var> var, VarVector* localVars, VarVector* globalVars);
  Type*   GetAndQualifyUnderlyingType(Type* type);
  void    ExtractPipelineVars(ClassType* classType, std::vector<Var*>* globalVars);
  Expr*   ExtractBuiltInVars(Type* type, Stmts* stmts, Stmts* postStmts);
  Expr*   CreateAndLoadInputVar(Type* type, std::string name);
  Expr*   CreateAndLoadInputVars(Type* type);
  Stmt*   CreateAndStoreOutputVar(Type* type, std::string name, Expr* value);
  void    CreateAndStoreOutputVars(Type* type, Expr* value, Stmts* stmts);
  Type*   GetWrapper(Type* type, int qualifiers);
  bool    IsWrapper(Type* type) const;

  TypeTable*              types_;
  Stmts*                  rootStmts_ = nullptr;
  MethodMap               methodMap_;
  VarAliasMap             varAliases_;
  UnfoldedVarMap          unfoldedVars_;
  UnfoldedPtrMap          unfoldedPtrs_;
  std::unique_ptr<Method> entryPointWrapper_;
  int                     methodModifiers_;
  VarVector               inputs_;
  VarVector               outputs_;
  BindGroupList           bindGroups_;
  VarVector               builtInVars_;
  WrapperMap              wrapper_;
  WrapperSet              wrappers_;
};

};  // namespace Toucan
#endif
