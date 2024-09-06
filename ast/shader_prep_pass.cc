// Copyright 2024 The Toucan Authors
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

#include "shader_prep_pass.h"

#include <ast/native_class.h>

namespace Toucan {

constexpr uint32_t kMaxBindGroups = 4;

ShaderPrepPass::ShaderPrepPass(NodeVector* nodes, TypeTable* types)
    : CopyVisitor(nodes), types_(types) {}

Result ShaderPrepPass::Visit(Stmts* stmts) {
  Stmts* newStmts = Make<Stmts>();

  if (!rootStmts_) { rootStmts_ = newStmts; }

  for (auto var : stmts->GetVars()) {
    rootStmts_->AppendVar(var);
  }

  for (Stmt* const& it : stmts->GetStmts()) {
    Stmt* stmt = Resolve(it);
    if (stmt) newStmts->Append(stmt);
  }
  return newStmts;
}

Type* ShaderPrepPass::GetAndQualifyUnderlyingType(Type* type) {
  assert(type->IsPtr());
  type = static_cast<PtrType*>(type)->GetBaseType();
  int   qualifiers;
  Type* unqualifiedType = types_->GetUnqualifiedType(type, &qualifiers);
  assert(unqualifiedType->IsClass());
  ClassType* classType = static_cast<ClassType*>(unqualifiedType);
  if (classType->GetTemplate() == NativeClass::Buffer) {
    assert(classType->GetTemplateArgs().size() == 1);
    Type* templateArgType = classType->GetTemplateArgs()[0];
    type = types_->GetQualifiedType(templateArgType, qualifiers);
    if (templateArgType->IsUnsizedArray()) {
      type = types_->GetWrapperClass(type);
      type = types_->GetQualifiedType(type, qualifiers);
    }
    return type;
  }
  return type;
}

void ShaderPrepPass::ExtractPipelineVars(Method* entryPoint) {
  if (entryPoint->modifiers & Method::STATIC) { return; }
  assert(entryPoint->formalArgList.size() > 0);
  ExtractPipelineVars(entryPoint->classType);
  assert(bindGroups_.size() <= kMaxBindGroups);
  // Remove this "this" pointer.
  entryPoint->formalArgList.erase(entryPoint->formalArgList.begin());
  entryPoint->modifiers |= Method::STATIC;
}

void ShaderPrepPass::ExtractPipelineVars(ClassType* classType) {
  if (classType->GetParent()) { ExtractPipelineVars(classType->GetParent()); }
  for (const auto& field : classType->GetFields()) {
    Type*    type = field->type;
    uint32_t pipelineVar = 0;

    assert(type->IsPtr());
    type = static_cast<PtrType*>(type)->GetBaseType();
    int   qualifiers;
    Type* unqualifiedType = type->GetUnqualifiedType(&qualifiers);
    assert(unqualifiedType->IsClass());
    ClassType* classType = static_cast<ClassType*>(unqualifiedType);
    if (classType->GetTemplate() == NativeClass::ColorAttachment) {
      if (shaderType_ == ShaderType::Fragment) {
        type = classType->GetTemplateArgs()[0];
        type = static_cast<ClassType*>(type)->FindType("SampledType");
        type = types_->GetVector(type, 4);
        outputs_.push_back(type);
        outputIndices_.push_back(field->index);
      }
    } else if (classType->GetTemplate() == NativeClass::DepthStencilAttachment) {
      // Do nothing; depth/stencil variables are inaccessible from device code.
    } else if (classType->GetTemplate() == NativeClass::Buffer &&
               qualifiers == Type::Qualifier::Vertex) {
      if (shaderType_ == ShaderType::Vertex) {
        Type* arg = classType->GetTemplateArgs()[0];
        assert(arg->IsArray());
        Type* elementType = static_cast<ArrayType*>(arg)->GetElementType();
        inputs_.push_back(elementType);
        inputIndices_.push_back(field->index);
      }
    } else if (classType->GetTemplate() == NativeClass::Buffer &&
               qualifiers == Type::Qualifier::Index) {
      // Do nothing; index buffers are inaccessible from device code.
    } else if (classType->GetTemplate() == NativeClass::BindGroup) {
      Type*     argType = classType->GetTemplateArgs()[0];
      VarVector bindGroup;
      if (argType->IsClass()) {
        auto* bindGroupClass = static_cast<ClassType*>(argType);
        for (auto& subField : bindGroupClass->GetFields()) {
          Type* type = GetAndQualifyUnderlyingType(subField->type);
          auto  var = std::make_shared<Var>(subField->name, type);
          bindGroup.push_back(var);
        }
      } else {
        Type* type = GetAndQualifyUnderlyingType(argType);
        auto  var = std::make_shared<Var>(field->name, type);
        bindGroup.push_back(var);
      }
      bindGroups_.push_back(bindGroup);
      bindGroupIndices_.push_back(field->index);
    }
  }
}

Method* ShaderPrepPass::Run(Method* entryPoint) {
  shaderType_ = entryPoint->shaderType;
  Method* newEntryPoint = PrepMethod(entryPoint);

  ExtractPipelineVars(entryPoint);
  return newEntryPoint;
}

Method* ShaderPrepPass::PrepMethod(Method* method) {
  if (methodMap_[method]) { return methodMap_[method].get(); }

  auto    newMethod = std::make_unique<Method>(*method);
  Method* result = newMethod.get();
  methodMap_[method] = std::move(newMethod);
  Stmts* prevRootStmts = rootStmts_;
  rootStmts_ = nullptr;
  result->stmts = Resolve(method->stmts);
  rootStmts_ = prevRootStmts;

  return result;
}

Result ShaderPrepPass::Visit(MethodCall* node) {
  Method*                   method = PrepMethod(node->GetMethod());
  const std::vector<Expr*>& args = node->GetArgList()->Get();
  auto*                     newArgs = Make<ExprList>();
  Stmts*                    writeStmts = nullptr;
  for (auto& i : args) {
    Expr* arg = Resolve(i);
    auto* type = arg->GetType(types_);
    if (type->IsPtr() && arg->IsFieldAccess() || arg->IsArrayAccess()) {
      auto* baseType = static_cast<PtrType*>(type)->GetBaseType();
      auto  var = std::make_shared<Var>("temp", baseType);
      rootStmts_->AppendVar(var);
      VarExpr* varExpr = Make<VarExpr>(var.get());
      if (baseType->IsWriteable()) {
        if (!writeStmts) writeStmts = Make<Stmts>();
        Expr* load = Make<LoadExpr>(varExpr);
        Stmt* store = Make<StoreStmt>(arg, load);
        writeStmts->Append(store);
      }
      if (baseType->IsReadable()) {
        Expr* load = Make<LoadExpr>(arg);
        Stmt* store = Make<StoreStmt>(varExpr, load);
        arg = Make<ExprWithStmt>(varExpr, store);
      } else {
        arg = varExpr;
      }
    }
    newArgs->Append(arg);
  }
  Expr* result = Make<MethodCall>(method, newArgs);
  if (writeStmts) result = Make<ExprWithStmt>(result, writeStmts);
  return result;
}

Result ShaderPrepPass::Visit(RawToWeakPtr* node) {
  // All pointers are raw pointers in SPIR-V.
  return Resolve(node->GetExpr());
}

Result ShaderPrepPass::Visit(ZeroInitStmt* node) {
  // All variables are zero-initialized already
  return nullptr;
}

Result ShaderPrepPass::Default(ASTNode* node) {
  assert(false);
  return nullptr;
}

};  // namespace Toucan
