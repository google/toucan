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

class ShaderPrepPass : public CopyVisitor {
 public:
  ShaderPrepPass(NodeVector* nodes, TypeTable* types);
  Method* Run(Method* method);
  Result  Visit(MethodCall* node) override;
  Result  Visit(RawToWeakPtr* node) override;
  Result  Visit(Stmts* node) override;
  Result  Visit(ZeroInitStmt* node) override;
  Result  Default(ASTNode* node) override;

 private:
  TypeTable* types_;
  Stmts*     rootStmts_ = nullptr;
  MethodMap  methodMap_;
};

};  // namespace Toucan
#endif
