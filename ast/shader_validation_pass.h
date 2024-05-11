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

#ifndef _AST_AST_SHADER_VALIDATION_PASS_H_
#define _AST_AST_SHADER_VALIDATION_PASS_H_

#include "ast.h"

namespace Toucan {

class SymbolTable;

class ShaderValidationPass : public Visitor {
 public:
  ShaderValidationPass(SymbolTable* symbols, TypeTable* types, Type* thisPtrType);
  Result            Visit(ArgList* node) override;
  Result            Visit(ArrayAccess* node) override;
  Result            Visit(BinOpNode* node) override;
  Result            Visit(BoolConstant* constant) override;
  Result            Visit(CastExpr* expr) override;
  Result            Visit(UnresolvedClassDefinition* defn) override;
  Result            Visit(DoStatement* stmt) override;
  Result            Visit(DoubleConstant* constant) override;
  Result            Visit(EnumConstant* node) override;
  Result            Visit(ExprStmt* exprStmt) override;
  Result            Visit(FieldAccess* constant) override;
  Result            Visit(FloatConstant* constant) override;
  Result            Visit(ForStatement* forStmt) override;
  Result            Visit(IfStatement* stmt) override;
  Result            Visit(IntConstant* constant) override;
  Result            Visit(NewArrayExpr* expr) override;
  Result            Visit(NewExpr* node) override;
  Result            Visit(NullConstant* constant) override;
  Result            Visit(ReturnStatement* stmt) override;
  Result            Visit(LoadExpr* node) override;
  Result            Visit(Stmts* stmts) override;
  Result            Visit(StoreStmt* node) override;
  Result            Visit(UIntConstant* constant) override;
  Result            Visit(UnaryOp* node) override;
  Result            Visit(UnresolvedInitializer* node) override;
  Result            Visit(UnresolvedDot* node) override;
  Result            Visit(UnresolvedIdentifier* node) override;
  Result            Visit(UnresolvedMethodCall* node) override;
  Result            Visit(UnresolvedStaticMethodCall* node) override;
  Result            Visit(VarDeclaration* decl) override;
  Result            Visit(WhileStatement* stmt) override;
  void              Error(ASTNode* node, const char* fmt, ...);
  Result            Default(ASTNode* node) override;
  const NodeVector& nodes() { return nodes_; }
  int               GetNumErrors() const { return numErrors_; }

 private:
  Result       Resolve(ASTNode* node);
  SymbolTable* symbols_;
  TypeTable*   types_;
  NodeVector   nodes_;
  ASTNode*     returnValue_;
  int          numErrors_;
  Type*        thisPtrType_;
};

};  // namespace Toucan
#endif
