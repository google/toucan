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

namespace Toucan {

using MethodMap = std::unordered_map<Method*, std::unique_ptr<Method>>;

using BindGroupList = std::vector<VarVector>;

class ShaderPrepPass : public CopyVisitor {
 public:
  ShaderPrepPass(NodeVector* nodes, TypeTable* types);
  Method*              Run(Method* entryPoint);
  Result               Visit(MethodCall* node) override;
  Result               Visit(RawToWeakPtr* node) override;
  Result               Visit(Stmts* node) override;
  Result               Visit(ZeroInitStmt* node) override;
  Result               Default(ASTNode* node) override;
  const VarVector&     GetInputs() const { return inputs_; }
  const VarVector&     GetOutputs() const { return outputs_; }
  const BindGroupList& GetBindGroups() const { return bindGroups_; }

  const std::vector<int>& GetInputIndices() const { return inputIndices_; }
  const std::vector<int>& GetOutputIndices() const { return outputIndices_; }
  const std::vector<int>& GetBindGroupIndices() const { return bindGroupIndices_; }
  const VarVector&        GetBuiltInVars() const { return builtInVars_; }

 private:
  Method* PrepMethod(Method* method);
  Type*   GetAndQualifyUnderlyingType(Type* type);
  void    ExtractPipelineVars(ClassType* classType);
  void    ExtractBuiltInVars(Type* type);
  Expr*   LoadInputVar(Type* type, std::string name);
  Expr*   LoadInputVars(Type* type);
  Stmt*   StoreOutputVar(Type* type, std::string name, Expr* value);
  void    StoreOutputVars(Type* type, Expr* value, Stmts* stmts);

  TypeTable*              types_;
  Stmts*                  rootStmts_ = nullptr;
  MethodMap               methodMap_;
  std::unique_ptr<Method> entryPointWrapper_;
  ShaderType              shaderType_;
  VarVector               inputs_;
  VarVector               outputs_;
  BindGroupList           bindGroups_;
  VarVector               builtInVars_;
  std::vector<int>        inputIndices_;
  std::vector<int>        outputIndices_;
  std::vector<int>        bindGroupIndices_;
};

};  // namespace Toucan
#endif
