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

#ifndef _AST_AST_NODE_VISITOR_H_
#define _AST_AST_NODE_VISITOR_H_

#include "ast.h"

namespace Toucan {

class CopyVisitor : public Visitor {
 public:
  CopyVisitor(NodeVector* nodes);
  virtual Type* ResolveType(Type* type);
  Result        Visit(Arg* node) override;
  Result        Visit(ArgList* node) override;
  Result        Visit(ArrayAccess* node) override;
  Result        Visit(BinOpNode* node) override;
  Result        Visit(BoolConstant* constant) override;
  Result        Visit(CastExpr* expr) override;
  Result        Visit(UnresolvedClassDefinition* defn) override;
  Result        Visit(DoStatement* stmt) override;
  Result        Visit(DoubleConstant* constant) override;
  Result        Visit(EnumConstant* node) override;
  Result        Visit(ExprList* node) override;
  Result        Visit(ExprStmt* exprStmt) override;
  Result        Visit(FloatConstant* constant) override;
  Result        Visit(ForStatement* forStmt) override;
  Result        Visit(IfStatement* stmt) override;
  Result        Visit(Initializer* node) override;
  Result        Visit(IntConstant* constant) override;
  Result        Visit(MethodCall* node) override;
  Result        Visit(NewArrayExpr* expr) override;
  Result        Visit(NewExpr* node) override;
  Result        Visit(UnresolvedNewExpr* node) override;
  Result        Visit(NullConstant* constant) override;
  Result        Visit(RawToWeakPtr* node) override;
  Result        Visit(ReturnStatement* stmt) override;
  Result        Visit(LoadExpr* node) override;
  Result        Visit(SmartToRawPtr* node) override;
  Result        Visit(Stmts* stmts) override;
  Result        Visit(StoreStmt* node) override;
  Result        Visit(UIntConstant* constant) override;
  Result        Visit(UnaryOp* node) override;
  Result        Visit(UnresolvedInitializer* node) override;
  Result        Visit(UnresolvedDot* node) override;
  Result        Visit(UnresolvedIdentifier* node) override;
  Result        Visit(UnresolvedListExpr* node) override;
  Result        Visit(UnresolvedMethodCall* node) override;
  Result        Visit(UnresolvedStaticMethodCall* node) override;
  Result        Visit(VarDeclaration* decl) override;
  Result        Visit(WhileStatement* stmt) override;
  Result        Default(ASTNode* node) override;
  template <typename T>
  T* Resolve(T* t) {
    if (t) {
      if (copyFileLocation_) {
        ScopedFileLocation scopedFile(&fileLocation_, t->GetFileLocation());
        return static_cast<T*>(t->Accept(this).p);
      } else {
        return static_cast<T*>(t->Accept(this).p);
      }
    } else {
      return nullptr;
    }
  }

 protected:
  template <typename T, typename... ARGS>
  T* Make(ARGS&&... args) {
    T* node = nodes_->Make<T>(std::forward<ARGS>(args)...);
    node->SetFileLocation(fileLocation_);
    return node;
  }
  FileLocation fileLocation_;
  bool         copyFileLocation_ = true;
  NodeVector*  nodes_;
};

};  // namespace Toucan
#endif
