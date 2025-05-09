// Copyright 2025 The Toucan Authors
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

#ifndef _AST_CONSTANT_FOLDER_H_
#define _AST_CONSTANT_FOLDER_H_

#include "ast.h"

namespace Toucan {

class SymbolTable;

class ConstantFolder : public Visitor {
 public:
  ConstantFolder(TypeTable* types, void* data);
  void Resolve(ASTNode* node);
  void Resolve(ASTNode* node, void* data);
  template<class T> void Store(T value);
  template<class T> void IntegralBinOp(BinOpNode::Op, void* lhs, void* rhs);
  template<class T> void FloatingPointBinOp(BinOpNode::Op, void* lhs, void* rhs);
  template<class T> void StoreUnaryOp(UnaryOp::Op, void* rhs);
  Result Visit(CastExpr* node) override;
  Result Visit(BinOpNode* node) override;
  Result Visit(BoolConstant* node) override;
  Result Visit(DoubleConstant* node) override;
  Result Visit(ExprList* node) override;
  Result Visit(FloatConstant* node) override;
  Result Visit(Initializer* node) override;
  Result Visit(IntConstant* node) override;
  Result Visit(UIntConstant* node) override;
  Result Visit(UnaryOp* node) override;
  Result Default(ASTNode* node) override;

 private:
  TypeTable*  types_;
  void*       data_;
};

};  // namespace Toucan
#endif
