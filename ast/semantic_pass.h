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

#ifndef _AST_AST_SEMANTIC_PASS_H_
#define _AST_AST_SEMANTIC_PASS_H_

#include "node_visitor.h"

namespace Toucan {

class SymbolTable;

class SemanticPass : public NodeVisitor {
 public:
  SemanticPass(NodeVector* nodes, SymbolTable* symbols, TypeTable* types);
  Result Visit(ArgList* node) override;
  Result Visit(ArrayAccess* node) override;
  Result Visit(BinOpNode* node) override;
  Result Visit(CastExpr* expr) override;
  Result Visit(UnresolvedClassDefinition* defn) override;
  Result Visit(Data* expr) override;
  Result Visit(DoStatement* stmt) override;
  Result Visit(ForStatement* forStmt) override;
  Result Visit(IfStatement* stmt) override;
  Result Visit(NewArrayExpr* expr) override;
  Result Visit(LoadExpr* node) override;
  Result Visit(SmartToRawPtr* expr) override;
  Result Visit(Stmts* stmts) override;
  Result Visit(IncDecExpr* node) override;
  Result Visit(StoreStmt* node) override;
  Result Visit(UnresolvedDot* node) override;
  Result Visit(UnresolvedIdentifier* node) override;
  Result Visit(UnresolvedInitializer* node) override;
  Result Visit(UnresolvedMethodCall* node) override;
  Result Visit(UnresolvedNewExpr* node) override;
  Result Visit(UnresolvedStaticMethodCall* node) override;
  Result Visit(VarDeclaration* decl) override;
  Result Visit(WhileStatement* stmt) override;
  Result Error(const char* fmt, ...);
  Result Default(ASTNode* node) override;
  int    GetNumErrors() const { return numErrors_; }

 private:
  Result ResolveMethodCall(Expr*       expr,
                           ClassType*  classType,
                           std::string id,
                           ArgList*    arglist);
  Expr*  ResolveListExpr(UnresolvedListExpr* node, Type* dstType);
  void   WidenArgList(std::vector<Expr*>& argList, const VarVector& formalArgList);
  Expr*  Widen(Expr* expr, Type* dstType);
  Stmt*  InitializeVar(Expr* varExpr, Type* type, Expr* initExpr);
  Stmts* InitializeClass(Expr* thisExpr, ClassType* classType);
  SymbolTable* symbols_;
  TypeTable*   types_;
  int          numErrors_;
};

};  // namespace Toucan
#endif
