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

#include "shader_validation_pass.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include <filesystem>

namespace Toucan {

ShaderValidationPass::ShaderValidationPass() {}

void ShaderValidationPass::Run(Method* method) {
  if (method->stmts) Visit(method->stmts);
}

Result ShaderValidationPass::Visit(ArrayAccess* node) {
  Resolve(node->GetExpr());
  Resolve(node->GetIndex());
  return {};
}

Result ShaderValidationPass::Visit(CastExpr* node) {
  Resolve(node->GetExpr());
  return {};
}

Result ShaderValidationPass::Visit(DestroyStmt* node) {
  Resolve(node->GetExpr());
  return {};
}

Result ShaderValidationPass::Visit(ExprWithStmt* node) {
  Resolve(node->GetExpr());
  Resolve(node->GetStmt());
  return {};
}

Result ShaderValidationPass::Visit(ExprList* node) {
  for (auto expr : node->Get()) {
    Resolve(expr);
  }
  return {};
}

Result ShaderValidationPass::Visit(ExtractElementExpr* node) {
  Resolve(node->GetExpr());
  return {};
}

Result ShaderValidationPass::Visit(IntConstant* node) { return {}; }

Result ShaderValidationPass::Visit(UIntConstant* node) { return {}; }

Result ShaderValidationPass::Visit(EnumConstant* node) { return {}; }

Result ShaderValidationPass::Visit(DoubleConstant* node) { return {}; }
 
Result ShaderValidationPass::Visit(FloatConstant* node) { return {}; }

Result ShaderValidationPass::Visit(HeapAllocation* node) {
  Error(node, "\"new\" operator is prohibited in shader methods");
  return {};
}

Result ShaderValidationPass::Visit(SliceExpr* node) {
  Error(node, "slice operator is prohibited in shader methods");
  return {};
}

Result ShaderValidationPass::Visit(Initializer* node) {
  Resolve(node->GetArgList());
  return {};
}

Result ShaderValidationPass::Visit(BoolConstant* node) { return {}; }

Result ShaderValidationPass::Visit(NullConstant* node) { return {}; }

Result ShaderValidationPass::Visit(Stmts* stmts) {
  for (Stmt* const& it : stmts->GetStmts()) {
    Resolve(it);
  }
  return {};
}

Result ShaderValidationPass::Visit(ArgList* a) {
  for (Arg* const& i : a->GetArgs()) {
    Resolve(i);
  }
  return {};
}

Result ShaderValidationPass::Visit(ExprStmt* stmt) {
  Resolve(stmt->GetExpr());
  return {};
}

Result ShaderValidationPass::Visit(LoadExpr* node) {
  Resolve(node->GetExpr());
  return {};
}

Result ShaderValidationPass::Visit(MethodCall* node) {
  Resolve(node->GetArgList());
  return {};
}

Result ShaderValidationPass::Visit(StoreStmt* node) {
  Resolve(node->GetLHS());
  Resolve(node->GetRHS());
  return {};
}

Result ShaderValidationPass::Visit(RawToSmartPtr* node) {
  Resolve(node->GetExpr());
  return {};
}

Result ShaderValidationPass::Visit(BinOpNode* node) {
  Resolve(node->GetRHS());
  Resolve(node->GetLHS());
  return {};
}

Result ShaderValidationPass::Visit(UnaryOp* node) {
  Resolve(node->GetRHS());
  return {};
}

Result ShaderValidationPass::Visit(ReturnStatement* stmt) {
  Resolve(stmt->GetExpr());
  return {};
}

Result ShaderValidationPass::Visit(IfStatement* s) {
  Resolve(s->GetExpr());
  Resolve(s->GetStmt());
  Resolve(s->GetOptElse());
  return {};
}

Result ShaderValidationPass::Visit(WhileStatement* s) {
  Resolve(s->GetCond());
  Resolve(s->GetBody());
  return {};
}

Result ShaderValidationPass::Visit(DoStatement* s) {
  Resolve(s->GetBody());
  Resolve(s->GetCond());
  return {};
}

Result ShaderValidationPass::Visit(ForStatement* node) {
  Resolve(node->GetInitStmt());
  Resolve(node->GetCond());
  Resolve(node->GetLoopStmt());
  Resolve(node->GetBody());
  return {};
}

Result ShaderValidationPass::Visit(FieldAccess* fieldAccess) {
  Resolve(fieldAccess->GetExpr());
  return {};
}

Result ShaderValidationPass::Visit(InsertElementExpr* node) {
  Resolve(node->GetExpr());
  Resolve(node->newElement());
  return {};
}

Result ShaderValidationPass::Visit(SmartToRawPtr* node) {
  Resolve(node->GetExpr());
  return {};
}

Result ShaderValidationPass::Visit(SwizzleExpr* node) {
  Resolve(node->GetExpr());
  return {};
}

Result ShaderValidationPass::Visit(TempVarExpr* node) {
  Resolve(node->GetInitExpr());
  return {};
}

Result ShaderValidationPass::Visit(ToRawArray* node) {
  Resolve(node->GetData());
  Resolve(node->GetLength());
  return {};
}

Result ShaderValidationPass::Visit(VarExpr* node) {
  return {};
}

Result ShaderValidationPass::Visit(ZeroInitStmt* node) {
  Resolve(node->GetLHS());
  return {};
}

Result ShaderValidationPass::Default(ASTNode* node) {
  assert(!"unhandled node");
  return {};
}

Result ShaderValidationPass::Resolve(ASTNode* node) { return node ? node->Accept(this) : nullptr; }

void ShaderValidationPass::Error(ASTNode* node, const char* fmt, ...) {
  const FileLocation& location = node->GetFileLocation();
  std::string         filename =
      location.filename ? std::filesystem::path(*location.filename).filename().string() : "";
  va_list argp;
  va_start(argp, fmt);
  fprintf(stderr, "%s:%d:  ", filename.c_str(), location.lineNum);
  vfprintf(stderr, fmt, argp);
  fprintf(stderr, "\n");
  numErrors_++;
}

};  // namespace Toucan
