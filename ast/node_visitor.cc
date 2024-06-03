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

Result NodeVisitor::Visit(Arg* node) { return Make<Arg>(node->GetID(), Resolve(node->GetExpr())); }

Result NodeVisitor::Visit(ArrayAccess* node) {
  Expr* expr = Resolve(node->GetExpr());
  Expr* index = Resolve(node->GetIndex());
  return Make<ArrayAccess>(expr, index);
}

Result NodeVisitor::Visit(CastExpr* node) {
  Type* type = ResolveType(node->GetType());
  Expr* expr = Resolve(node->GetExpr());
  return Make<CastExpr>(type, expr);
}

Result NodeVisitor::Visit(Initializer* node) {
  Type*     type = ResolveType(node->GetType());
  ExprList* argList = Resolve(node->GetArgList());
  return Make<Initializer>(type, argList);
}

Result NodeVisitor::Visit(IntConstant* node) {
  return Make<IntConstant>(node->GetValue(), node->GetBits());
}

Result NodeVisitor::Visit(UIntConstant* node) {
  return Make<UIntConstant>(node->GetValue(), node->GetBits());
}

Result NodeVisitor::Visit(EnumConstant* node) { return Make<EnumConstant>(node->GetValue()); }

Result NodeVisitor::Visit(FloatConstant* node) { return Make<FloatConstant>(node->GetValue()); }

Result NodeVisitor::Visit(DoubleConstant* node) { return Make<DoubleConstant>(node->GetValue()); }

Result NodeVisitor::Visit(BoolConstant* node) { return Make<BoolConstant>(node->GetValue()); }

Result NodeVisitor::Visit(NullConstant* node) { return Make<NullConstant>(); }

Result NodeVisitor::Visit(Stmts* stmts) {
  auto* newStmts = Make<Stmts>();
  for (Stmt* const& it : stmts->GetStmts()) {
    Stmt* stmt = Resolve(it);
    if (stmt) newStmts->Append(Resolve(stmt));
  }
  return newStmts;
}

Result NodeVisitor::Visit(ArgList* a) {
  auto* arglist = Make<ArgList>();
  for (Arg* const& i : a->GetArgs()) {
    arglist->Append(Resolve(i));
  }
  return arglist;
}

Result NodeVisitor::Visit(ExprList* node) {
  auto* exprList = Make<ExprList>();
  for (auto expr : node->Get()) {
    exprList->Append(Resolve(expr));
  }
  return exprList;
}

Result NodeVisitor::Visit(ExprStmt* stmt) { return Make<ExprStmt>(Resolve(stmt->GetExpr())); }

Result NodeVisitor::Visit(UnresolvedInitializer* node) {
  Type*    type = ResolveType(node->GetType());
  ArgList* argList = Resolve(node->GetArgList());
  return Make<UnresolvedInitializer>(type, argList, node->IsConstructor());
}

Result NodeVisitor::Visit(VarDeclaration* decl) {
  Type* type = ResolveType(decl->GetType());
  Expr* initExpr = Resolve(decl->GetInitExpr());
  return Make<VarDeclaration>(decl->GetID(), type, initExpr);
}

Result NodeVisitor::Visit(LoadExpr* node) { return Make<LoadExpr>(Resolve(node->GetExpr())); }

Result NodeVisitor::Visit(SmartToRawPtr* node) {
  return Make<SmartToRawPtr>(Resolve(node->GetExpr()));
}

Result NodeVisitor::Visit(StoreStmt* node) {
  return Make<StoreStmt>(Resolve(node->GetLHS()), Resolve(node->GetRHS()));
}

Result NodeVisitor::Visit(BinOpNode* node) {
  Expr* rhs = Resolve(node->GetRHS());
  Expr* lhs = Resolve(node->GetLHS());
  return Make<BinOpNode>(node->GetOp(), lhs, rhs);
}

Result NodeVisitor::Visit(UnaryOp* node) {
  Expr* rhs = Resolve(node->GetRHS());
  if (!rhs) return nullptr;
  return Make<UnaryOp>(node->GetOp(), rhs);
}

Result NodeVisitor::Visit(RawToWeakPtr* node) {
  return Make<RawToWeakPtr>(Resolve(node->GetExpr()));
}

Result NodeVisitor::Visit(ReturnStatement* stmt) {
  return Make<ReturnStatement>(Resolve(stmt->GetExpr()), stmt->GetScope());
}

Result NodeVisitor::Visit(NewExpr* expr) {
  Type*     type = ResolveType(expr->GetType());
  Expr*     length = Resolve(expr->GetLength());
  ExprList* args = Resolve(expr->GetArgs());
  return Make<NewExpr>(type, length, expr->GetConstructor(), args);
}

Result NodeVisitor::Visit(IfStatement* s) {
  Expr* expr = Resolve(s->GetExpr());
  Stmt* stmt = Resolve(s->GetStmt());
  Stmt* optElse = Resolve(s->GetOptElse());
  return Make<IfStatement>(expr, stmt, optElse);
}

Result NodeVisitor::Visit(WhileStatement* s) {
  Expr* cond = Resolve(s->GetCond());
  Stmt* body = Resolve(s->GetBody());
  return Make<WhileStatement>(cond, body);
}

Result NodeVisitor::Visit(DoStatement* s) {
  Stmt* body = Resolve(s->GetBody());
  Expr* cond = Resolve(s->GetCond());
  return Make<DoStatement>(body, cond);
}

Result NodeVisitor::Visit(ForStatement* node) {
  Stmt* initStmt = Resolve(node->GetInitStmt());
  Expr* cond = Resolve(node->GetCond());
  Stmt* loopStmt = Resolve(node->GetLoopStmt());
  Stmt* body = Resolve(node->GetBody());
  return Make<ForStatement>(initStmt, cond, loopStmt, body);
}

Result NodeVisitor::Visit(NewArrayExpr* expr) {
  Type* type = ResolveType(expr->GetElementType());
  Expr* sizeExpr = Resolve(expr->GetSizeExpr());
  return Make<NewArrayExpr>(type, sizeExpr);
}

Result NodeVisitor::Visit(UnresolvedClassDefinition* defn) {
  return Make<UnresolvedClassDefinition>(defn->GetScope());
}

Result NodeVisitor::Visit(UnresolvedDot* node) {
  return Make<UnresolvedDot>(Resolve(node->GetExpr()), node->GetID());
}

Result NodeVisitor::Visit(UnresolvedListExpr* node) {
  return Make<UnresolvedListExpr>(Resolve(node->GetArgList()));
}

Result NodeVisitor::Visit(UnresolvedMethodCall* node) {
  return Make<UnresolvedMethodCall>(Resolve(node->GetExpr()), node->GetID(),
                                    Resolve(node->GetArgList()));
}

Result NodeVisitor::Visit(UnresolvedNewExpr* expr) {
  Type*    type = ResolveType(expr->GetType());
  Expr*    length = Resolve(expr->GetLength());
  ArgList* arglist = Resolve(expr->GetArgList());
  return Make<UnresolvedNewExpr>(type, length, arglist);
}

Result NodeVisitor::Visit(UnresolvedStaticMethodCall* node) {
  return Make<UnresolvedStaticMethodCall>(node->classType(), node->GetID(),
                                          Resolve(node->GetArgList()));
}

Result NodeVisitor::Visit(UnresolvedIdentifier* node) {
  return Make<UnresolvedIdentifier>(node->GetID());
}

Result NodeVisitor::Default(ASTNode* node) {
  fprintf(stderr, "Internal error: unhandled node in NodeVisitor pass.");
  return {};
}

};  // namespace Toucan
