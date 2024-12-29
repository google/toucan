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

#include "dump_as_source_pass.h"

#include <stdarg.h>

#include "symbol.h"

namespace Toucan {

DumpAsSourcePass::DumpAsSourcePass(FILE* file, std::unordered_map<Type*, int>* typeMap)
    : file_(file), typeMap_(typeMap) {
  map_[nullptr] = 0;
}

#define NOTIMPLEMENTED() assert(!"that node is not implemented")

int DumpAsSourcePass::Resolve(ASTNode* node) {
  if (!map_[node]) { node->Accept(this); }
  return map_[node];
}

int DumpAsSourcePass::Output(ASTNode* node, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(file_, "  nodeList[%d] = nodes->", nodeCount_);
  vfprintf(file_, fmt, ap);
  fprintf(file_, ";\n");
  map_[node] = nodeCount_;
  return nodeCount_++;
}

Result DumpAsSourcePass::Visit(ArrayAccess* node) {
  int expr = Resolve(node->GetExpr());
  int index = Resolve(node->GetIndex());
  Output(node, "Make<ArrayAccess>(nodeList[%d], nodeList[%d])", expr, index);
  return {};
}

Result DumpAsSourcePass::Visit(CastExpr* node) {
  int type = (*typeMap_)[node->GetType()];
  int expr = Resolve(node->GetExpr());
  Output(node, "Make<CastExpr>(typeList[%d], exprs[%d])", type, expr);
  return {};
}

Result DumpAsSourcePass::Visit(IntConstant* node) {
  Output(node, "Make<IntConstant>(%d, %d)", node->GetValue(), node->GetBits());
  return {};
}

Result DumpAsSourcePass::Visit(UIntConstant* node) {
  Output(node, "Make<UIntConstant>(%u, %d)", node->GetValue(), node->GetBits());
  return {};
}

Result DumpAsSourcePass::Visit(EnumConstant* node) {
  Output(node, "Make<EnumConstant>(%d)", node->GetValue()->value);
  return {};
}

Result DumpAsSourcePass::Visit(FloatConstant* node) {
  Output(node, "Make<FloatConstant>(%g)", node->GetValue());
  return {};
}

Result DumpAsSourcePass::Visit(DoubleConstant* node) {
  Output(node, "Make<DoubleConstant>(%lg)", node->GetValue());
  return {};
}

Result DumpAsSourcePass::Visit(BoolConstant* node) {
  Output(node, "Make<BoolConstant>(%s)", node->GetValue() ? "true" : "false");
  return {};
}

Result DumpAsSourcePass::Visit(NullConstant* node) {
  Output(node, "Make<NullConstant>()");
  return {};
}

Result DumpAsSourcePass::Visit(Stmts* stmts) {
  int id = Output(stmts, "Make<Stmts>()");
  if (stmts->GetScope()) {
    fprintf(file_, "  stmtss[%d]->SetScope(symbols->PushNewScope());\n", id);
  }
  // FIXME: create an actual Stmts from elements
  for (Stmt* const& it : stmts->GetStmts()) {
    fprintf(file_, "  stmtss[%d]->Append(stmts[%d]);\n", id, Resolve(it));
  }
  if (stmts->GetScope()) { fprintf(file_, "  symbols->PopScope();\n"); }
  return {};
}

Result DumpAsSourcePass::Visit(ExprList* a) {
  int id = Output(a, "Make<ExprList>()");
  for (auto expr : a->Get()) {
    fprintf(file_, "  exprLists[%d]->Append(exprs[%d]);\n", id, Resolve(expr));
  }
  return {};
}

Result DumpAsSourcePass::Visit(ExprStmt* stmt) {
  Output(stmt, "Make<ExprStmt>(exprs[%d])", Resolve(stmt->GetExpr()));
  return {};
}

Result DumpAsSourcePass::Visit(Initializer* node) {
  int type = (*typeMap_)[node->GetType()];
  int argList = Resolve(node->GetArgList());
  Output(node, "Make<Initializer>(typeList[%d], exprLists[%d])", type, argList);
  return {};
}

Result DumpAsSourcePass::Visit(VarDeclaration* decl) {
  int type = (*typeMap_)[decl->GetType()];
  int initExpr = Resolve(decl->GetInitExpr());
  Output(decl, "Make<VarDeclaration>(\"%s\", typeList[%d], exprs[%d])", decl->GetID().c_str(), type,
         initExpr);
  return {};
}

Result DumpAsSourcePass::Visit(LoadExpr* node) {
  int expr = Resolve(node->GetExpr());
  Output(node, "Make<LoadExpr>(exprs[%d])", expr);
  return {};
}

Result DumpAsSourcePass::Visit(StoreStmt* node) {
  int lhs = Resolve(node->GetLHS());
  int rhs = Resolve(node->GetRHS());
  Output(node, "Make<StoreStmt>(exprs[%d], exprs[%d])", lhs, rhs);
  return {};
}

static const char* GetOp(BinOpNode::Op op) {
  switch (op) {
    case BinOpNode::ADD: return "ADD";
    case BinOpNode::SUB: return "SUB";
    case BinOpNode::MUL: return "MUL";
    case BinOpNode::DIV: return "DIV";
    case BinOpNode::LT: return "LT";
    case BinOpNode::LE: return "LE";
    case BinOpNode::GE: return "GE";
    case BinOpNode::GT: return "GT";
    case BinOpNode::NE: return "NE";
    case BinOpNode::BITWISE_AND: return "BITWISE_AND";
    case BinOpNode::BITWISE_OR: return "BITWISE_OR";
    default: assert(false); return "";
  }
}

Result DumpAsSourcePass::Visit(BinOpNode* node) {
  int lhs = Resolve(node->GetLHS());
  int rhs = Resolve(node->GetRHS());
  Output(node, "Make<BinOpNode>(BinOpNode::%s, exprs[%d], exprs[%d])", GetOp(node->GetOp()), lhs,
         rhs);
  return {};
}

Result DumpAsSourcePass::Visit(UnaryOp* node) {
  NOTIMPLEMENTED();
  return {};
}

Result DumpAsSourcePass::Visit(ReturnStatement* stmt) {
  if (stmt->GetExpr()) {
    int expr = Resolve(stmt->GetExpr());
    Output(stmt, "Make<ReturnStatement>(exprs[%d])", expr);
  } else {
    Output(stmt, "Make<ReturnStatement>(nullptr)");
  }
  return {};
}

Result DumpAsSourcePass::Visit(IfStatement* s) {
  NOTIMPLEMENTED();
  return {};
}

Result DumpAsSourcePass::Visit(WhileStatement* s) {
  NOTIMPLEMENTED();
  return {};
}

Result DumpAsSourcePass::Visit(DoStatement* s) {
  NOTIMPLEMENTED();
  return {};
}

Result DumpAsSourcePass::Visit(ForStatement* node) {
  NOTIMPLEMENTED();
  return {};
}

Result DumpAsSourcePass::Default(ASTNode* node) {
  NOTIMPLEMENTED();
  return {};
}

};  // namespace Toucan
