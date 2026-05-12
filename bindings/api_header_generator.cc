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

#include "api_header_generator.h"

#include <sstream>
#include <unordered_set>

#include <ast/name_mangler.h>
#include <ast/constant_folder.h>

namespace Toucan {

namespace {

// FIXME: refactor this with type.cc's version (or remove that one)
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

APIHeaderGenerator::APIHeaderGenerator(Stmts* stmts, TypeTable* types, std::ostream& header) : stmts_(stmts), types_(types), header_(header) {}

void APIHeaderGenerator::Run() {
  header_ << "#include <cstdint>\n";
  header_ << "extern \"C\" {\n";
  header_ << "namespace Toucan {\n\n";
  header_ << "class ClassType;\n";
  header_ << "class Type;\n";
  header_ << "using Deleter = void(*)(void*);\n\n";
  header_ << "struct ControlBlock {\n";
  header_ << "  uint32_t    strongRefs = 0;\n";
  header_ << "  uint32_t    weakRefs = 0;\n";
  header_ << "  uint32_t    arrayLength;\n";
  header_ << "  Type*       type = nullptr;\n";
  header_ << "  Deleter     deleter = nullptr;\n";
  header_ << "};\n\n";
  header_ << "struct Object {\n";
  header_ << "  void*          ptr;\n";
  header_ << "  ControlBlock  *controlBlock;\n";
  header_ << "};\n\n";
  header_ << "struct Array {\n";
  header_ << "  void*          ptr;\n";
  header_ << "  uint32_t       length;\n";
  header_ << "};\n\n";
  emitMethods_ = false;
  for (auto& node : stmts_->GetStmts()) node->Accept(this);
  emitMethods_ = true;
  for (auto& node : stmts_->GetStmts()) node->Accept(this);
  header_ << "\n};\n}\n";
}

Result APIHeaderGenerator::Visit(IntConstant* node) {
  header_ << "int" << node->GetBits() << "_t";
  return {};
}

Result APIHeaderGenerator::Visit(UIntConstant* node) {
  header_ << "uint" << node->GetBits() << "_t";
  return {};
}

Result APIHeaderGenerator::Visit(ClassDecl* node) {
  currentClassDecl_ = node;
  if (emitMethods_ ) {
    overloadedMethodNames_ = OverloadFinder::Run(node->GetBody());
    node->GetBody()->Accept(this);
  } else {
    auto prevBuf = header_.rdbuf();
    auto body = std::stringstream();
    header_.rdbuf(body.rdbuf());
    node->GetBody()->Accept(this);
    header_.rdbuf(prevBuf);
    header_ << "struct " << node->GetName();
    if (!body.str().empty()) {
      header_ << " {\n" << body.str() << "}";
    }
    header_ << ";\n";
  }
  currentClassDecl_ = nullptr;
  return {};
}

Result APIHeaderGenerator::Visit(EnumDecl* node) {
  if (emitMethods_ ) return {};
  header_ << "enum class " << node->GetName() << " {\n";
  for (auto value : node->GetValues()->GetValues()) value->Accept(this);
  header_ << "};\n";
  return {};
}

Result APIHeaderGenerator::Visit(ASTEnumValue* node) {
  header_ << "  " << node->GetID();
  if (node->GetValue().has_value()) header_ << " = " << node->GetValue().value();
  header_ << ",\n";
  return {};
}

Result APIHeaderGenerator::Visit(FloatConstant* node) {
  header_ << "float";
  return {};
}

Result APIHeaderGenerator::Visit(DoubleConstant* node) {
  header_ << "double";
  return {};
}

Result APIHeaderGenerator::Visit(BoolConstant* node) {
  header_ << "bool";
  return {};
}

Result APIHeaderGenerator::Visit(Stmts* stmts) {
  for (auto stmt : stmts->GetStmts()) {
    stmt->Accept(this);
  }
  return {};
}

Result APIHeaderGenerator::Visit(ReturnStatement* node) {
  return {};
}

Result APIHeaderGenerator::Visit(UnresolvedInitializer* node) {
  node->GetType()->Accept(this);
  return {};
}

Result APIHeaderGenerator::Visit(VarDeclaration* node) {
  currentAutoExpr_ = node->GetInitExpr();
  if (emitMethods_ ) {
    if (!currentMethodDecl_) return {};
    currentVarID_ = node->GetID();
    if (currentVarID_ == "this") currentVarID_ = "This";
    node->GetType()->Accept(this);
    WriteCurrentVarID();
  } else { 
    header_ << "  ";
    currentVarID_ = node->GetID();
    node->GetType()->Accept(this);
    WriteCurrentVarID();
    header_ << ";\n";
  }
  currentAutoExpr_ = nullptr;
  return {};
}

Result APIHeaderGenerator::Visit(ConstDecl* node) {
  return {};
}

Result APIHeaderGenerator::Visit(MethodDecl* node) {
  if (!emitMethods_) return {};
  if (node->GetModifiers() & Method::Modifier::DeviceOnly) return {};
  auto className = currentClassDecl_->GetName();
  bool isConstructor = node->GetID() == className;
  bool hasThisPtr = !(node->GetModifiers() & Method::Modifier::Static) && !isConstructor;
#if TARGET_OS_IS_WIN
  header_ << "__declspec(dllexport) ";
#endif
  node->GetReturnType()->Accept(this);
  auto formalArgs = node->GetFormalArguments();
  currentMethodDecl_ = node;
  bool isOverloaded = overloadedMethodNames_.contains(node->GetID());
  header_ << " "<< GetMangledName(currentClassDecl_, node, isOverloaded) << "(";
  if (node->GetID() == className && currentClassDecl_->IsTemplate()) {
    header_ << "int qualifiers, ";
    auto templateClassDecl = static_cast<ClassDecl*>(currentClassDecl_);
    for (ASTFormalTemplateArg* arg : templateClassDecl->GetFormalTemplateArgs()->Get()) {
      header_ << "Type* " << arg->GetName();
      if (arg != templateClassDecl->GetFormalTemplateArgs()->Get().back() || (formalArgs && !formalArgs->GetStmts().empty())) {
        header_ << ", ";
      }
    }
  }
  if (hasThisPtr) {
    header_ << className << "* This";
    if (!formalArgs->GetStmts().empty()) {
      header_ << ", ";
    }
  }
  for (auto& arg : formalArgs->GetStmts()) {
    arg->Accept(this);
    if (&arg != &node->GetFormalArguments()->GetStmts().back()) { header_ << ", "; }
  }
  
  currentMethodDecl_ = nullptr;
  header_ << ");\n";
  return {};
}

Result APIHeaderGenerator::Visit(Decls* decls) {
  for (auto decl : decls->Get()) {
    decl->Accept(this);
  }
  return {};
}

Result APIHeaderGenerator::Visit(LoadExpr* node) {
  node->GetExpr()->Accept(this);
  return {};
}


Result APIHeaderGenerator::Visit(TempVarExpr* node) {
  node->GetInitExpr()->Accept(this);
  return {};
}

Result APIHeaderGenerator::Visit(UnresolvedStaticDot* node) {
  node->GetType()->Accept(this);
  return {};
}

void APIHeaderGenerator::WriteCurrentVarID() {
  if (!currentVarID_.empty()) {
    header_ << " " << currentVarID_;
    currentVarID_.clear();
  }
}

void APIHeaderGenerator::WriteCurrentIndices() {
  for (auto index : currentIndices_) {
    header_ << "[" << index << "]";
  }
  currentIndices_.clear();
}

Result APIHeaderGenerator::Visit(ASTArrayType* node) {
  int numElements = 0;
  if (node->GetNumElements()) {
    ConstantFolder constantFolder(types_, &numElements);
    constantFolder.Resolve(node->GetNumElements());
  }
  if (rawPointerContext_ && numElements == 0) {
    header_ << "Array";
    return {};
  }
  currentIndices_.push_back(numElements);
  node->GetElementType()->Accept(this);
  WriteCurrentVarID();
  WriteCurrentIndices();

  return {};
}

Result APIHeaderGenerator::Visit(ASTAutoType* node) {
  assert(currentAutoExpr_);
  currentAutoExpr_->Accept(this);
  return {};
}

Result APIHeaderGenerator::Visit(ASTBoolType* node) {
  header_ << "bool";
  return {};
}

Result APIHeaderGenerator::Visit(ASTClassType* node) {
  header_ << node->GetDecl()->GetName();
  return {};
}

Result APIHeaderGenerator::Visit(ASTClassTemplateInstance* node) {
  assert(node->GetClassTemplate()->IsClass());
  header_ << static_cast<ASTClassType*>(node->GetClassTemplate())->GetDecl()->GetName();
  return {};
}

Result APIHeaderGenerator::Visit(ASTFloatingPointType* node) {
  if (node->GetBits() == 32) {
    header_ << "float";
  } else if (node->GetBits() == 64) {
    header_ << "double";
  } else {
    assert(!"unknown floating point type in header generator");
  }

  return {};
}

Result APIHeaderGenerator::Visit(ASTFormalTemplateArg* node) {
  header_ << "void";
  return {};
}

Result APIHeaderGenerator::Visit(ASTIntegerType* node) {
  if (!node->IsSigned()) header_ << "u";
  header_ << "int" << node->GetBits() << "_t";

  return {};
}

Result APIHeaderGenerator::Visit(ASTMatrixType* node) {
  if (emitMethods_) {
    header_ << "const ";
    node->GetColumnType()->GetComponentType()->Accept(this);
    header_ << "*";
  } else {
    currentIndices_.push_back(node->GetNumColumns());
    node->GetColumnType()->Accept(this);
    WriteCurrentVarID();
    WriteCurrentIndices();
  }
  return {};
}

Result APIHeaderGenerator::Visit(ASTQualifiedType* node) {
  node->GetBaseType()->Accept(this);
  
  return {};
}

Result APIHeaderGenerator::Visit(ASTRawPtrType* node) {
  rawPointerContext_ = true;
  node->GetBaseType()->Accept(this);
  rawPointerContext_ = false;
  header_ << "*";

  return {};
}

Result APIHeaderGenerator::Visit(ASTStrongPtrType* node) {
  auto baseType = node->GetBaseType()->GetUnqualifiedType();
  if (baseType->IsClass() || baseType->IsClassTemplateInstance()) {
    node->GetBaseType()->Accept(this);
  } else {
    header_ << "Object";
  }
  header_ << "*";

  return {};
}

Result APIHeaderGenerator::Visit(ASTVectorType* node) {
  if (emitMethods_) {
    header_ << "const ";
    node->GetComponentType()->Accept(this);
    header_ << "*";
  } else {
    currentIndices_.push_back(node->GetNumComponents());
    node->GetComponentType()->Accept(this);
    WriteCurrentVarID();
    WriteCurrentIndices();
  }
  return {};
}

Result APIHeaderGenerator::Visit(ASTVoidType* node) {
  header_ << "void";
  return {};
}

Result APIHeaderGenerator::Visit(ASTEnumType* node) {
  header_ << node->GetDecl()->GetName();
  return {};
}

Result APIHeaderGenerator::Default(ASTNode* node) {
  assert(!"unhandled node in APIHeaderGenerator");
  return {};
}

};  // namespace Toucan
