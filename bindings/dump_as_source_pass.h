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

#ifndef _AST_DUMP_AS_SOURCE_PASS_H_
#define _AST_DUMP_AS_SOURCE_PASS_H_

#include "ast/ast.h"

#include <ostream>

namespace Toucan {

class GenBindings;

class DumpAsSourcePass : public Visitor {
 public:
  DumpAsSourcePass(std::ostream& file, GenBindings* genBindings);
  int    Resolve(ASTNode* node);
  std::ostream& Output(ASTNode* node);
  Result Visit(ArgList* node) override;
  Result Visit(ArrayAccess* node) override;
  Result Visit(BinOpNode* node) override;
  Result Visit(BoolConstant* constant) override;
  Result Visit(CastExpr* expr) override;
  Result Visit(EnumConstant* node) override;
  Result Visit(ExprList* exprList) override;
  Result Visit(DoubleConstant* constant) override;
  Result Visit(FloatConstant* constant) override;
  Result Visit(Initializer* node) override;
  Result Visit(IntConstant* constant) override;
  Result Visit(NullConstant* constant) override;
  Result Visit(LoadExpr* node) override;
  Result Visit(UIntConstant* constant) override;
  Result Visit(UnresolvedListExpr* node) override;
  Result Default(ASTNode* node) override;

 private:
  std::ostream&                     file_;
  std::unordered_map<ASTNode*, int> map_;
  GenBindings*                      genBindings_;
  int                               nodeCount_ = 1;
};

};  // namespace Toucan
#endif
