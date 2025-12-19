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
#include "bindings/gen_bindings.h"

#include <stdarg.h>

namespace Toucan {

DumpAsSourcePass::DumpAsSourcePass(std::ostream& file, GenBindings* genBindings)
    : file_(file), genBindings_(genBindings) {
  map_[nullptr] = 0;
}

int DumpAsSourcePass::Resolve(ASTNode* node) {
  if (!map_[node]) { node->Accept(this); }
  return map_[node];
}

std::ostream& DumpAsSourcePass::Output(ASTNode* node) {
  file_ << "  auto* node" << nodeCount_ << " = nodes->";
  map_[node] = nodeCount_++;
  return file_;
}

Result DumpAsSourcePass::Visit(ArgList* node) {
  // For now, support only empty ArgList.
  assert(node->GetArgs().size() == 0);
  Output(node) << "Make<ArgList>();\n";
  return {};
}

Result DumpAsSourcePass::Visit(ArrayAccess* node) {
  int expr = Resolve(node->GetExpr());
  int index = Resolve(node->GetIndex());
  Output(node) << "Make<ArrayAccess>(node" << expr << ", node" << index << ");\n";
  return {};
}

Result DumpAsSourcePass::Visit(CastExpr* node) {
  int type = genBindings_->EmitType(node->GetType());
  int expr = Resolve(node->GetExpr());
  Output(node) << "Make<CastExpr>(type" << type << ", node" << expr << ");\n";
  return {};
}

Result DumpAsSourcePass::Visit(IntConstant* node) {
  Output(node) << "Make<IntConstant>(" << node->GetValue() << ", " << node->GetBits() << ");\n";
  return {};
}

Result DumpAsSourcePass::Visit(UIntConstant* node) {
  Output(node) << "Make<UIntConstant>(" << node->GetValue() << ", " << node->GetBits() << ");\n";
  return {};
}

Result DumpAsSourcePass::Visit(EnumConstant* node) {
  const EnumValue* value = node->GetValue();
  int type = genBindings_->EmitType(value->type);
  Output(node) << "Make<EnumConstant>(type" << type << "->FindValue(\"" << value->id << "\"));\n";
  return {};
}

Result DumpAsSourcePass::Visit(FloatConstant* node) {
  Output(node) << "Make<FloatConstant>(" << node->GetValue() << ");\n";
  return {};
}

Result DumpAsSourcePass::Visit(DoubleConstant* node) {
  Output(node) << "Make<DoubleConstant>(" << node->GetValue() << ");\n";
  return {};
}

Result DumpAsSourcePass::Visit(BoolConstant* node) {
  Output(node) << "Make<BoolConstant>(" << (node->GetValue() ? "true" : "false") << ");\n";
  return {};
}

Result DumpAsSourcePass::Visit(NullConstant* node) {
  Output(node) << "Make<NullConstant>();\n";
  return {};
}

Result DumpAsSourcePass::Visit(ExprList* a) {
  Output(a) << "Make<ExprList>();\n";
  int id = map_[a];
  for (auto expr : a->Get()) {
    int exprID = Resolve(expr);
    file_ << "  node" << id << "->Append(node" << exprID << ");\n";
  }
  return {};
}

Result DumpAsSourcePass::Visit(Initializer* node) {
  int type = genBindings_->EmitType(node->GetType());
  int argList = Resolve(node->GetArgList());
  Output(node) << "Make<Initializer>(type" << type << ", node" << argList << ");\n";
  return {};
}

Result DumpAsSourcePass::Visit(LoadExpr* node) {
  int expr = Resolve(node->GetExpr());
  Output(node) << "Make<LoadExpr>(node" << expr << ");\n";
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
  Output(node) << "Make<BinOpNode>(BinOpNode::" << GetOp(node->GetOp()) << ", node" << lhs
               << ", node" << rhs << ");\n";
  return {};
}


Result DumpAsSourcePass::Visit(UnresolvedListExpr* node) {
  int argList = Resolve(node->GetArgList());
  Output(node) << "Make<UnresolvedListExpr>(node" << argList << ");\n";
  return {};
}

Result DumpAsSourcePass::Default(ASTNode* node) {
  assert(!"that node is not implemented");
  return {};
}

};  // namespace Toucan
