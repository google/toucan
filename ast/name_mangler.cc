// Copyright 2026 The Toucan Authors
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

#include "name_mangler.h"

#include "ast/ast.h"
#include "ast/type.h"

namespace Toucan {

namespace {

std::string QualifiersToString(int qualifiers, std::string sep) {
  std::string result;
  if (qualifiers & Type::Qualifier::Uniform) { result += "uniform" + sep; }
  if (qualifiers & Type::Qualifier::Storage) { result += "storage" + sep; }
  if (qualifiers & Type::Qualifier::Vertex) { result += "vertex" + sep; }
  if (qualifiers & Type::Qualifier::Index) { result += "index" + sep; }
  if (qualifiers & Type::Qualifier::Sampleable) { result += "sampleable" + sep; }
  if (qualifiers & Type::Qualifier::Renderable) { result += "renderable" + sep; }
  if (qualifiers & Type::Qualifier::ReadOnly) { result += "readonly" + sep; }
  if (qualifiers & Type::Qualifier::WriteOnly) { result += "writeonly" + sep; }
  if (qualifiers & Type::Qualifier::HostReadable) { result += "hostreadable" + sep; }
  if (qualifiers & Type::Qualifier::HostWriteable) { result += "hostwriteable" + sep; }
  if (qualifiers & Type::Qualifier::Coherent) { result += "coherent" + sep; }
  if (qualifiers & Type::Qualifier::Unfilterable) { result += "unfilterable" + sep; }
  return result;
}

class ArgToString : public Visitor {
 public:
  ArgToString() {}
  Result Visit(VarDeclaration* node) override {
    node->GetType()->Accept(this);
    return {};
  }
  Result Visit(ASTClassType* node) override {
    result_ += node->GetDecl()->GetName();
    return {};
  }
  Result Visit(ASTClassTemplateInstance* node) override {
    node->GetClassTemplate()->Accept(this);
    return {};
  }
  Result Visit(ASTFormalTemplateArg* node) override {
    result_ += node->GetName();
    return {};
  }
  Result Visit(ASTRawPtrType* node) override {
    node->GetBaseType()->Accept(this);
    return {};
  }
  Result Visit(ASTWeakPtrType* node) override {
    node->GetBaseType()->Accept(this);
    return {};
  }
  Result Visit(ASTStrongPtrType* node) override {
    node->GetBaseType()->Accept(this);
    return {};
  }
  Result Visit(ASTQualifiedType* node) override {
    result_ += QualifiersToString(node->GetQualifiers(), "_");
    node->GetBaseType()->Accept(this);
    return {};
  }
  Result Visit(ASTBoolType* node) override {
    result_ += "bool";
    return {};
  }
  Result Visit(ASTIntegerType* node) override {
    result_ += node->IsSigned() ? "int" : "uint";
    return {};
  }
  Result Visit(ASTFloatingPointType* node) override {
    result_ += node->GetBits() == 32 ? "float" : "double";
    return {};
  }
  Result Visit(ASTVectorType* node) override {
    node->GetComponentType()->Accept(this);
    result_ += std::to_string(node->GetNumComponents());
    return {};
  }
  Result Default(ASTNode* node) override {
    assert(!"unhandled node type in ArgToString visitor");
    return {};
  }
  static std::string Run(Stmt* arg) {
    ArgToString argToString;
    arg->Accept(&argToString);
    return std::move(argToString.result_);
  }
 private:
  std::string result_;
};

}

std::string GetMangledName(ClassDecl* classDecl, MethodDecl* method, bool overloaded) {
  std::string methodName = method->GetID();
  if (methodName[0] == '~') methodName = "Destroy";
  std::string result = classDecl->GetName() + "_" + methodName;
  if (overloaded) {
    for (auto arg : method->GetFormalArguments()->GetStmts()) {
      result += "_" + ArgToString::Run(arg);
    }
  }
  return result;
}

}
