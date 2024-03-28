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

#include "node_visitor.h"

namespace Toucan {

NodeVisitor::NodeVisitor(NodeVector* nodes) : nodes_(nodes) {}

Type* NodeVisitor::ResolveType(Type* type) { return type; }

Result NodeVisitor::Visit(Arg* node) { return Make<Arg>(node, node->GetID(), Resolve(node->GetExpr())); }

Result NodeVisitor::Visit(ArrayAccess* node) {
  Expr* expr = Resolve(node->GetExpr());
  Expr* index = Resolve(node->GetIndex());
  return Make<ArrayAccess>(node, expr, index);
}

Result NodeVisitor::Visit(CastExpr* node) {
  Type* type = ResolveType(node->GetType());
  Expr* expr = Resolve(node->GetExpr());
  return Make<CastExpr>(node, type, expr);
}

Result NodeVisitor::Visit(IntConstant* node) {
  return Make<IntConstant>(node, node->GetValue(), node->GetBits());
}

Result NodeVisitor::Visit(UIntConstant* node) {
  return Make<UIntConstant>(node, node->GetValue(), node->GetBits());
}

Result NodeVisitor::Visit(EnumConstant* node) { return Make<EnumConstant>(node, node->GetValue()); }

Result NodeVisitor::Visit(FloatConstant* node) { return Make<FloatConstant>(node, node->GetValue()); }

Result NodeVisitor::Visit(DoubleConstant* node) {
  return Make<DoubleConstant>(node, node->GetValue());
}

Result NodeVisitor::Visit(BoolConstant* node) { return Make<BoolConstant>(node, node->GetValue()); }

Result NodeVisitor::Visit(NullConstant* node) { return Make<NullConstant>(node); }

Result NodeVisitor::Visit(Stmts* stmts) {
  auto* newStmts = Make<Stmts>(stmts);
  for (Stmt* const& it : stmts->GetStmts()) {
    Stmt* stmt = Resolve(it);
    if (stmt) newStmts->Append(Resolve(stmt));
  }
  return newStmts;
}

Result NodeVisitor::Visit(ArgList* a) {
  auto* arglist = Make<ArgList>(a);
  for (Arg* const& i : a->GetArgs()) {
    arglist->Append(Resolve(i));
  }
  return arglist;
}

Result NodeVisitor::Visit(ExprStmt* stmt) { return Make<ExprStmt>(stmt, Resolve(stmt->GetExpr())); }

Result NodeVisitor::Visit(ConstructorNode* node) {
  Type*    type = ResolveType(node->GetType());
  ArgList* argList = Resolve(node->GetArgList());
  return Make<ConstructorNode>(node, type, argList);
}

Result NodeVisitor::Visit(VarDeclaration* decl) {
  Type* type = ResolveType(decl->GetType());
  Expr* initExpr = Resolve(decl->GetInitExpr());
  return Make<VarDeclaration>(decl, decl->GetID(), type, initExpr);
}

Result NodeVisitor::Visit(LoadExpr* node) { return Make<LoadExpr>(node, Resolve(node->GetExpr())); }

Result NodeVisitor::Visit(StoreStmt* node) {
  return Make<StoreStmt>(node, Resolve(node->GetLHS()), Resolve(node->GetRHS()));
}

Result NodeVisitor::Visit(BinOpNode* node) {
  Expr* rhs = Resolve(node->GetRHS());
  Expr* lhs = Resolve(node->GetLHS());
  return Make<BinOpNode>(node, node->GetOp(), lhs, rhs);
}

Result NodeVisitor::Visit(UnaryOp* node) {
  Expr* rhs = Resolve(node->GetRHS());
  if (!rhs) return nullptr;
  return Make<UnaryOp>(node, node->GetOp(), rhs);
}

Result NodeVisitor::Visit(ReturnStatement* stmt) {
  return Make<ReturnStatement>(stmt, Resolve(stmt->GetExpr()), stmt->GetScope());
}

Result NodeVisitor::Visit(NewExpr* expr) {
  Type*     type = ResolveType(expr->GetType());
  Expr*     length = Resolve(expr->GetLength());
  ExprList* args = Resolve(expr->GetArgs());
  return Make<NewExpr>(expr, type, length, expr->GetConstructor(), args);
}

Result NodeVisitor::Visit(IfStatement* s) {
  Expr* expr = Resolve(s->GetExpr());
  Stmt* stmt = Resolve(s->GetStmt());
  Stmt* optElse = Resolve(s->GetOptElse());
  return Make<IfStatement>(s, expr, stmt, optElse);
}

Result NodeVisitor::Visit(WhileStatement* s) {
  Expr* cond = Resolve(s->GetCond());
  Stmt* body = Resolve(s->GetBody());
  return Make<WhileStatement>(s, cond, body);
}

Result NodeVisitor::Visit(DoStatement* s) {
  Stmt* body = Resolve(s->GetBody());
  Expr* cond = Resolve(s->GetCond());
  return Make<DoStatement>(s, body, cond);
}

Result NodeVisitor::Visit(ForStatement* node) {
  Stmt* initStmt = Resolve(node->GetInitStmt());
  Expr* cond = Resolve(node->GetCond());
  Stmt* loopStmt = Resolve(node->GetLoopStmt());
  Stmt* body = Resolve(node->GetBody());
  return Make<ForStatement>(node, initStmt, cond, loopStmt, body);
}

Result NodeVisitor::Visit(NewArrayExpr* expr) {
  Type* type = ResolveType(expr->GetElementType());
  Expr* sizeExpr = Resolve(expr->GetSizeExpr());
  return Make<NewArrayExpr>(expr, type, sizeExpr);
}

Result NodeVisitor::Visit(UnresolvedClassDefinition* defn) {
  return Make<UnresolvedClassDefinition>(defn, defn->GetScope());
}

Result NodeVisitor::Visit(UnresolvedDot* node) {
  return Make<UnresolvedDot>(node, Resolve(node->GetExpr()), node->GetID());
}

Result NodeVisitor::Visit(UnresolvedListExpr* node) {
  return Make<UnresolvedListExpr>(node, Resolve(node->GetArgList()));
}

Result NodeVisitor::Visit(UnresolvedMethodCall* node) {
  return Make<UnresolvedMethodCall>(node, Resolve(node->GetExpr()), node->GetID(),
                                    Resolve(node->GetArgList()));
}

Result NodeVisitor::Visit(UnresolvedNewExpr* expr) {
  Type*    type = ResolveType(expr->GetType());
  Expr*    length = Resolve(expr->GetLength());
  ArgList* arglist = Resolve(expr->GetArgList());
  return Make<UnresolvedNewExpr>(expr, type, length, arglist);
}

Result NodeVisitor::Visit(UnresolvedStaticMethodCall* node) {
  return Make<UnresolvedStaticMethodCall>(node, node->classType(), node->GetID(),
                                          Resolve(node->GetArgList()));
}

Result NodeVisitor::Visit(UnresolvedIdentifier* node) {
  return Make<UnresolvedIdentifier>(node, node->GetID());
}

Result NodeVisitor::Default(ASTNode* node) {
  fprintf(stderr, "Internal error: unhandled node in NodeVisitor pass.");
  return {};
}

};  // namespace Toucan
