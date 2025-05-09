
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

#include "constant_folder.h"

#ifdef _WIN32
#include <malloc.h>
#define alloca _alloca
#endif

namespace Toucan {

ConstantFolder::ConstantFolder(TypeTable* types, void* data) : types_(types), data_(data) {
}

void ConstantFolder::Resolve(ASTNode* node) {
  node->Accept(this);
}

void ConstantFolder::Resolve(ASTNode* node, void* data) {
  auto prevData = data_;
  data_ = data;
  node->Accept(this);
  data_ = prevData;
}

Result ConstantFolder::Visit(CastExpr* node) {
  assert(node->IsTransparent(types_));
  Resolve(node->GetExpr());
  return {};
}

template <class T> void ConstantFolder::Store(T value) {
  *static_cast<T*>(data_) = value;
}

Result ConstantFolder::Visit(IntConstant* node) {
  switch (node->GetBits()) {
    case 8:
      Store<int8_t>(node->GetValue()); break;
    case 16:
      Store<int16_t>(node->GetValue()); break;
    case 32:
      Store<int32_t>(node->GetValue()); break;
    default:
      assert(!"unsupported bit width");
  }
  return {};
}

Result ConstantFolder::Visit(UIntConstant* node) {
  switch (node->GetBits()) {
    case 8:
      Store<uint8_t>(node->GetValue()); break;
    case 16:
      Store<uint16_t>(node->GetValue()); break;
    case 32:
      Store<uint32_t>(node->GetValue()); break;
    default:
      assert(!"unsupported bit width");
  }
  return {};
}

Result ConstantFolder::Visit(FloatConstant* node) {
  Store<float>(node->GetValue());
  return {};
}

Result ConstantFolder::Visit(DoubleConstant* node) {
  Store<double>(node->GetValue());
  return {};
}

Result ConstantFolder::Visit(BoolConstant* node) {
  Store<uint8_t>(node->GetValue() ? 1 : 0);
  return {};
}

Result ConstantFolder::Visit(Initializer* node) {
  Resolve(node->GetArgList());
  return {};
}

template <class T> void ConstantFolder::IntegralBinOp(BinOpNode::Op op, void* lhs, void* rhs) {
  auto l = *static_cast<T*>(lhs);
  auto r = *static_cast<T*>(rhs);
  switch (op) {
    case BinOpNode::ADD:           Store<T>(l + r); break;
    case BinOpNode::SUB:           Store<T>(l - r); break;
    case BinOpNode::MUL:           Store<T>(l * r); break;
    case BinOpNode::DIV:           Store<T>(l / r); break;
    case BinOpNode::MOD:           Store<T>(l % r); break;
    case BinOpNode::LT:            Store<T>(l < r); break;
    case BinOpNode::LE:            Store<T>(l <= r); break;
    case BinOpNode::EQ:            Store<T>(l == r); break;
    case BinOpNode::GE:            Store<T>(l >= r); break;
    case BinOpNode::GT:            Store<T>(l > r); break;
    case BinOpNode::NE:            Store<T>(l != r); break;
    case BinOpNode::LOGICAL_AND:   Store<T>(l && r); break;
    case BinOpNode::BITWISE_AND:   Store<T>(l & r); break;
    case BinOpNode::LOGICAL_OR:    Store<T>(l || r); break;
    case BinOpNode::BITWISE_OR:    Store<T>(l | r); break;
    case BinOpNode::BITWISE_XOR:   Store<T>(l ^ r); break;
    default: assert(false);
  }
}

template <class T> void ConstantFolder::FloatingPointBinOp(BinOpNode::Op op, void* lhs, void* rhs) {
  auto l = *static_cast<T*>(lhs);
  auto r = *static_cast<T*>(rhs);
  switch (op) {
    case BinOpNode::ADD:           Store<T>(l + r); break;
    case BinOpNode::SUB:           Store<T>(l - r); break;
    case BinOpNode::MUL:           Store<T>(l * r); break;
    case BinOpNode::DIV:           Store<T>(l / r); break;
    case BinOpNode::LT:            Store<T>(l < r); break;
    case BinOpNode::LE:            Store<T>(l <= r); break;
    case BinOpNode::EQ:            Store<T>(l == r); break;
    case BinOpNode::GE:            Store<T>(l >= r); break;
    case BinOpNode::GT:            Store<T>(l > r); break;
    case BinOpNode::NE:            Store<T>(l != r); break;
    default: assert(false);
  }
}

template <class T> void ConstantFolder::StoreUnaryOp(UnaryOp::Op op, void *rhs) {
  auto r = *static_cast<T*>(rhs);
  switch (op) {
    case UnaryOp::Op::Minus:       Store<T>(-r); break;
    case UnaryOp::Op::Negate:      Store<T>(!r); break;
    default: assert(false);
  }
}

Result ConstantFolder::Visit(BinOpNode* node) {
  auto lhsType = node->GetLHS()->GetType(types_);
  auto rhsType = node->GetLHS()->GetType(types_);
  auto lhs = alloca(lhsType->GetSizeInBytes());
  auto rhs = alloca(rhsType->GetSizeInBytes());
  Resolve(node->GetLHS(), lhs);
  Resolve(node->GetRHS(), rhs);
  if (lhsType->IsInt()) {
    IntegralBinOp<int32_t>(node->GetOp(), lhs, rhs);
  } else if (lhsType->IsUInt()) {
    IntegralBinOp<uint32_t>(node->GetOp(), lhs, rhs);
  } else if (lhsType->IsShort()) {
    IntegralBinOp<int16_t>(node->GetOp(), lhs, rhs);
  } else if (lhsType->IsUShort()) {
    IntegralBinOp<uint16_t>(node->GetOp(), lhs, rhs);
  } else if (lhsType->IsByte()) {
    IntegralBinOp<int8_t>(node->GetOp(), lhs, rhs);
  } else if (lhsType->IsUByte()) {
    IntegralBinOp<uint8_t>(node->GetOp(), lhs, rhs);
  } else if (lhsType->IsFloat()) {
    FloatingPointBinOp<float>(node->GetOp(), lhs, rhs);
  } else if (lhsType->IsDouble()) {
    FloatingPointBinOp<double>(node->GetOp(), lhs, rhs);
  } else {
    assert(false);
  }
  return {};
}

Result ConstantFolder::Visit(UnaryOp* node) {
  auto rhsType = node->GetRHS()->GetType(types_);
  auto rhs = alloca(rhsType->GetSizeInBytes());
  Resolve(node->GetRHS(), rhs);
  if (rhsType->IsInt()) {
    StoreUnaryOp<int32_t>(node->GetOp(), rhs);
  } else if (rhsType->IsUInt()) {
    StoreUnaryOp<uint32_t>(node->GetOp(), rhs);
  } else if (rhsType->IsShort()) {
    StoreUnaryOp<int16_t>(node->GetOp(), rhs);
  } else if (rhsType->IsUShort()) {
    StoreUnaryOp<uint16_t>(node->GetOp(), rhs);
  } else if (rhsType->IsByte()) {
    StoreUnaryOp<int8_t>(node->GetOp(), rhs);
  } else if (rhsType->IsUByte()) {
    StoreUnaryOp<uint8_t>(node->GetOp(), rhs);
  } else if (rhsType->IsFloat()) {
    StoreUnaryOp<float>(node->GetOp(), rhs);
  } else if (rhsType->IsDouble()) {
    StoreUnaryOp<double>(node->GetOp(), rhs);
  } else {
    assert(false);
  }
  return {};
}

Result ConstantFolder::Visit(ExprList* node) {
  auto d = static_cast<char*>(data_);
  for (auto expr : node->Get()) {
    Resolve(expr, d);
    d += expr->GetType(types_)->GetSizeInBytes();
  }
  return {};
}

Result ConstantFolder::Default(ASTNode* node) {
  assert(!"that node is not implemented");
  return {};
}

};  // namespace Toucan
