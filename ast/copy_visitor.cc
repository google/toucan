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

#include "copy_visitor.h"

#define RESOLVE_OR_DIE(result, value) \
  auto result = Resolve(value); \
  if (!result) return nullptr;


namespace Toucan {

CopyVisitor::CopyVisitor(NodeVector* nodes) : nodes_(nodes) {}

Type* CopyVisitor::ResolveType(Type* type) { return type; }

Result CopyVisitor::Visit(Arg* node) {
  RESOLVE_OR_DIE(expr, node->GetExpr());

  return Make<Arg>(node->GetID(), expr, node->IsUnfold());
}

Result CopyVisitor::Visit(ArrayAccess* node) {
  RESOLVE_OR_DIE(expr, node->GetExpr());
  RESOLVE_OR_DIE(index, node->GetIndex());

  return Make<ArrayAccess>(expr, index);
}

Result CopyVisitor::Visit(CastExpr* node) {
  Type* type = ResolveType(node->GetType());
  RESOLVE_OR_DIE(expr, node->GetExpr());

  return Make<CastExpr>(type, expr);
}

Result CopyVisitor::Visit(ExprWithStmt* node) {
  RESOLVE_OR_DIE(expr, node->GetExpr());
  Stmt* stmt = Resolve(node->GetStmt());

  return Make<ExprWithStmt>(expr, stmt);
}

Result CopyVisitor::Visit(ExtractElementExpr* node) {
  RESOLVE_OR_DIE(expr, node->GetExpr());

  return Make<ExtractElementExpr>(expr, node->GetIndex());
}

Result CopyVisitor::Visit(Initializer* node) {
  Type*     type = ResolveType(node->GetType());
  RESOLVE_OR_DIE(argList, node->GetArgList());

  return Make<Initializer>(type, argList);
}

Result CopyVisitor::Visit(IntConstant* node) {
  return Make<IntConstant>(node->GetValue(), node->GetBits());
}

Result CopyVisitor::Visit(UIntConstant* node) {
  return Make<UIntConstant>(node->GetValue(), node->GetBits());
}

Result CopyVisitor::Visit(EnumConstant* node) { return Make<EnumConstant>(node->GetValue()); }

Result CopyVisitor::Visit(FloatConstant* node) { return Make<FloatConstant>(node->GetValue()); }

Result CopyVisitor::Visit(DoubleConstant* node) { return Make<DoubleConstant>(node->GetValue()); }

Result CopyVisitor::Visit(BoolConstant* node) { return Make<BoolConstant>(node->GetValue()); }

Result CopyVisitor::Visit(NullConstant* node) { return Make<NullConstant>(); }

Result CopyVisitor::Visit(Stmts* stmts) {
  auto* newStmts = Make<Stmts>();
  for (Stmt* const& it : stmts->GetStmts()) {
    Stmt* stmt = Resolve(it);
    if (stmt) newStmts->Append(Resolve(stmt));
  }
  for (auto var : stmts->GetVars()) {
    newStmts->AppendVar(var);
  }
  return newStmts;
}

Result CopyVisitor::Visit(ArgList* a) {
  auto* argList = Make<ArgList>();
  for (Arg* const& arg : a->GetArgs()) {
    RESOLVE_OR_DIE(result, arg);

    argList->Append(result);
  }
  return argList;
}

Result CopyVisitor::Visit(ExprList* node) {
  auto* exprList = Make<ExprList>();
  for (auto i : node->Get()) {
    RESOLVE_OR_DIE(expr, i);
    exprList->Append(expr);
  }
  return exprList;
}

Result CopyVisitor::Visit(ExprStmt* stmt) {
  RESOLVE_OR_DIE(expr, stmt->GetExpr());

  return Make<ExprStmt>(expr);
}

Result CopyVisitor::Visit(InsertElementExpr* node) {
  RESOLVE_OR_DIE(expr, node->GetExpr());
  RESOLVE_OR_DIE(newElement, node->newElement());

  return Make<InsertElementExpr>(expr, newElement, node->GetIndex());
}

Result CopyVisitor::Visit(UnresolvedInitializer* node) {
  Type*    type = ResolveType(node->GetType());
  RESOLVE_OR_DIE(argList, node->GetArgList());

  return Make<UnresolvedInitializer>(type, argList, node->IsConstructor());
}

Result CopyVisitor::Visit(VarDeclaration* decl) {
  Type* type = ResolveType(decl->GetType());
  Expr* initExpr = Resolve(decl->GetInitExpr());

  return Make<VarDeclaration>(decl->GetID(), type, initExpr);
}

Result CopyVisitor::Visit(LoadExpr* node) {
  RESOLVE_OR_DIE(expr, node->GetExpr());

  return Make<LoadExpr>(expr);
}

Result CopyVisitor::Visit(RawToSmartPtr* node) {
  RESOLVE_OR_DIE(expr, node->GetExpr());

  return Make<RawToSmartPtr>(expr);
}

Result CopyVisitor::Visit(SliceExpr* node) {
  RESOLVE_OR_DIE(expr, node->GetExpr());
  auto start = Resolve(node->GetStart());
  auto end = Resolve(node->GetEnd());

  return Make<SliceExpr>(expr, start, end);
}

Result CopyVisitor::Visit(SmartToRawPtr* node) {
  RESOLVE_OR_DIE(expr, node->GetExpr());

  return Make<SmartToRawPtr>(expr);
}

Result CopyVisitor::Visit(StoreStmt* node) {
  RESOLVE_OR_DIE(lhs, node->GetLHS());
  RESOLVE_OR_DIE(rhs, node->GetRHS());

  return Make<StoreStmt>(lhs, rhs);
}

Result CopyVisitor::Visit(SwizzleExpr* node) {
  RESOLVE_OR_DIE(expr, node->GetExpr());

  return Make<SwizzleExpr>(expr, node->GetIndices());
}

Result CopyVisitor::Visit(TempVarExpr* node) {
  return Make<TempVarExpr>(ResolveType(node->GetType()), Resolve(node->GetInitExpr()));
}

Result CopyVisitor::Visit(BinOpNode* node) {
  RESOLVE_OR_DIE(rhs, node->GetRHS());
  RESOLVE_OR_DIE(lhs, node->GetLHS());

  return Make<BinOpNode>(node->GetOp(), lhs, rhs);
}

Result CopyVisitor::Visit(UnaryOp* node) {
  RESOLVE_OR_DIE(rhs, node->GetRHS());

  return Make<UnaryOp>(node->GetOp(), rhs);
}

Result CopyVisitor::Visit(IncDecExpr* node) {
  RESOLVE_OR_DIE(expr, node->GetExpr());

  return Make<IncDecExpr>(node->GetOp(), expr, node->returnOrigValue());
}

Result CopyVisitor::Visit(DestroyStmt* node) {
  RESOLVE_OR_DIE(expr, node->GetExpr());

  return Make<DestroyStmt>(expr);
}

Result CopyVisitor::Visit(ReturnStatement* stmt) {
  return Make<ReturnStatement>(Resolve(stmt->GetExpr()));
}

Result CopyVisitor::Visit(HeapAllocation* node) {
  return Make<HeapAllocation>(node->GetType());
}

Result CopyVisitor::Visit(IfStatement* s) {
  RESOLVE_OR_DIE(expr, s->GetExpr());
  Stmt* stmt = Resolve(s->GetStmt());
  Stmt* optElse = Resolve(s->GetOptElse());

  return Make<IfStatement>(expr, stmt, optElse);
}

Result CopyVisitor::Visit(WhileStatement* s) {
  RESOLVE_OR_DIE(cond, s->GetCond());
  Stmt* body = Resolve(s->GetBody());

  return Make<WhileStatement>(cond, body);
}

Result CopyVisitor::Visit(DoStatement* s) {
  Stmt* body = Resolve(s->GetBody());
  RESOLVE_OR_DIE(cond, s->GetCond());

  return Make<DoStatement>(body, cond);
}

Result CopyVisitor::Visit(ForStatement* node) {
  Stmt* initStmt = Resolve(node->GetInitStmt());
  RESOLVE_OR_DIE(cond, node->GetCond());
  Stmt* loopStmt = Resolve(node->GetLoopStmt());
  Stmt* body = Resolve(node->GetBody());
  return Make<ForStatement>(initStmt, cond, loopStmt, body);
}

Result CopyVisitor::Visit(MethodCall* node) {
  RESOLVE_OR_DIE(argList, node->GetArgList());
  return Make<MethodCall>(node->GetMethod(), argList);
}

Result CopyVisitor::Visit(UnresolvedClassDefinition* defn) {
  return Make<UnresolvedClassDefinition>(defn->GetScope());
}

Result CopyVisitor::Visit(UnresolvedDot* node) {
  RESOLVE_OR_DIE(expr, node->GetExpr());

  return Make<UnresolvedDot>(expr, node->GetID());
}

Result CopyVisitor::Visit(UnresolvedListExpr* node) {
  RESOLVE_OR_DIE(argList, node->GetArgList());

  return Make<UnresolvedListExpr>(argList);
}

Result CopyVisitor::Visit(UnresolvedMethodCall* node) {
  RESOLVE_OR_DIE(expr, node->GetExpr());
  RESOLVE_OR_DIE(argList, node->GetArgList());

  return Make<UnresolvedMethodCall>(expr, node->GetID(), argList);
}

Result CopyVisitor::Visit(UnresolvedNewExpr* expr) {
  Type*    type = ResolveType(expr->GetType());
  Expr*    length = Resolve(expr->GetLength());
  RESOLVE_OR_DIE(argList, expr->GetArgList());

  return Make<UnresolvedNewExpr>(type, length, argList, expr->IsConstructor());
}

Result CopyVisitor::Visit(UnresolvedStaticMethodCall* node) {
  RESOLVE_OR_DIE(argList, node->GetArgList());

  return Make<UnresolvedStaticMethodCall>(node->classType(), node->GetID(), argList);
}

Result CopyVisitor::Visit(UnresolvedIdentifier* node) {
  return Make<UnresolvedIdentifier>(node->GetID());
}

Result CopyVisitor::Visit(ZeroInitStmt* node) {
  RESOLVE_OR_DIE(lhs, node->GetLHS());

  return Make<ZeroInitStmt>(lhs);
}

Result CopyVisitor::Visit(VarExpr* node) {
  return Make<VarExpr>(node->GetVar());
}

Result CopyVisitor::Visit(FieldAccess* node) {
  RESOLVE_OR_DIE(expr, node->GetExpr());

  return Make<FieldAccess>(expr, node->GetField());
}

Result CopyVisitor::Default(ASTNode* node) {
  fprintf(stderr, "Internal error: unhandled node in CopyVisitor pass.");
  return {};
}

};  // namespace Toucan
