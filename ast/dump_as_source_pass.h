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

#include "ast.h"

#ifdef __gnuc__
#define CHECK_FORMAT(archetype, string_index, first_to_check) \
  __attribute__((format(archetype, string_index, first_to_check)))
#else
#define CHECK_FORMAT(...)
#endif

namespace Toucan {

class SymbolTable;

class DumpAsSourcePass : public Visitor {
 public:
  DumpAsSourcePass(FILE* file, std::unordered_map<Type*, int>* typeMap);
  int    Resolve(ASTNode* node);
  int    Output(ASTNode* node, const char* fmt, ...) CHECK_FORMAT(printf, 3, 4);
  Result Visit(Arg* node) override;
  Result Visit(ArgList* node) override;
  Result Visit(ArrayAccess* node) override;
  Result Visit(BinOpNode* node) override;
  Result Visit(BoolConstant* constant) override;
  Result Visit(CastExpr* expr) override;
  Result Visit(UnresolvedClassDefinition* defn) override;
  Result Visit(ConstructorNode* node) override;
  Result Visit(DoStatement* stmt) override;
  Result Visit(EnumConstant* node) override;
  Result Visit(ExprStmt* exprStmt) override;
  Result Visit(DoubleConstant* constant) override;
  Result Visit(FloatConstant* constant) override;
  Result Visit(ForStatement* forStmt) override;
  Result Visit(IfStatement* stmt) override;
  Result Visit(IntConstant* constant) override;
  Result Visit(NewArrayExpr* expr) override;
  Result Visit(NewExpr* node) override;
  Result Visit(NullConstant* constant) override;
  Result Visit(ReturnStatement* stmt) override;
  Result Visit(LoadExpr* node) override;
  Result Visit(Stmts* stmts) override;
  Result Visit(StoreStmt* node) override;
  Result Visit(UIntConstant* constant) override;
  Result Visit(UnaryOp* node) override;
  Result Visit(UnresolvedDot* node) override;
  Result Visit(UnresolvedIdentifier* node) override;
  Result Visit(UnresolvedMethodCall* node) override;
  Result Visit(UnresolvedNewExpr* node) override;
  Result Visit(UnresolvedStaticMethodCall* node) override;
  Result Visit(VarDeclaration* decl) override;
  Result Visit(WhileStatement* stmt) override;
  Result Default(ASTNode* node) override;

 private:
  FILE*                             file_;
  std::unordered_map<ASTNode*, int> map_;
  std::unordered_map<Type*, int>*   typeMap_;
  int                               nodeCount_ = 1;
};

};  // namespace Toucan
#endif
