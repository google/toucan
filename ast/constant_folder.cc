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

#include "constant_folder.h"

namespace Toucan {

ConstantFolder::ConstantFolder() : success_(true) {}

int ConstantFolder::Resolve(Expr* expr) {
  if (expr) {
    return expr->Accept(this).i;
  } else {
    return 0;
  }
}

Result ConstantFolder::Visit(IntConstant* node) { return static_cast<uint32_t>(node->GetValue()); }

Result ConstantFolder::Visit(UIntConstant* node) { return node->GetValue(); }

Result ConstantFolder::Visit(BinOpNode* node) {
  switch (node->GetOp()) {
    case BinOpNode::ADD: return Resolve(node->GetLHS()) + Resolve(node->GetRHS());
    case BinOpNode::SUB: return Resolve(node->GetLHS()) - Resolve(node->GetRHS());
    case BinOpNode::MUL: return Resolve(node->GetLHS()) * Resolve(node->GetRHS());
    case BinOpNode::DIV: return Resolve(node->GetLHS()) / Resolve(node->GetRHS());
    case BinOpNode::MOD: return Resolve(node->GetLHS()) % Resolve(node->GetRHS());
    case BinOpNode::LT: return Resolve(node->GetLHS()) < Resolve(node->GetRHS());
    case BinOpNode::LE: return Resolve(node->GetLHS()) <= Resolve(node->GetRHS());
    case BinOpNode::GT: return Resolve(node->GetLHS()) > Resolve(node->GetRHS());
    case BinOpNode::GE: return Resolve(node->GetLHS()) >= Resolve(node->GetRHS());
    case BinOpNode::EQ: return Resolve(node->GetLHS()) == Resolve(node->GetRHS());
    case BinOpNode::NE: return Resolve(node->GetLHS()) != Resolve(node->GetRHS());
    case BinOpNode::BITWISE_AND: return Resolve(node->GetLHS()) & Resolve(node->GetRHS());
    case BinOpNode::BITWISE_XOR: return Resolve(node->GetLHS()) ^ Resolve(node->GetRHS());
    case BinOpNode::BITWISE_OR: return Resolve(node->GetLHS()) | Resolve(node->GetRHS());
    case BinOpNode::LOGICAL_AND: return Resolve(node->GetLHS()) && Resolve(node->GetRHS());
    case BinOpNode::LOGICAL_OR: return Resolve(node->GetLHS()) || Resolve(node->GetRHS());
  }
}

Result ConstantFolder::Default(ASTNode* node) {
  success_ = false;
  return {};
}

};  // namespace Toucan
