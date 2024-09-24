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
      type = GetWrapper(type, qualifiers);
    }
    return type;
  }
  return type;
}

void ShaderPrepPass::ExtractPipelineVars(ClassType* classType) {
  if (classType->GetParent()) { ExtractPipelineVars(classType->GetParent()); }
  for (const auto& field : classType->GetFields()) {
    Type* type = field->type;

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
        outputs_.push_back(std::make_shared<Var>(field->name, type));
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
        inputs_.push_back(std::make_shared<Var>(field->name, elementType));
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

void ShaderPrepPass::ExtractBuiltInVars(Type* type) {
  assert(type->IsClass());
  auto classType = static_cast<ClassType*>(type);
  for (const auto& field : classType->GetFields()) {
    auto var = std::make_shared<Var>(field->name, field->type);
    builtInVars_.push_back(var);
  }
}

Expr* ShaderPrepPass::LoadInputVar(Type* type, std::string name) {
  auto var = std::make_shared<Var>(name, type);
  inputs_.push_back(var);
  inputIndices_.push_back(-1);
  return Make<LoadExpr>(Make<VarExpr>(var.get()));
}

// Given a class, creates Vars for each of its fields and adds them to inputss_.
// Loads the globals and constructs an instance of the class via Initialize().
Expr* ShaderPrepPass::LoadInputVars(Type* type) {
  if (type->IsClass()) {
    auto classType = static_cast<ClassType*>(type);
    auto args = Make<ExprList>();
    for (auto& i : classType->GetFields()) {
      Field* field = i.get();
      args->Append(LoadInputVar(field->type, field->name));
    }
    return Make<Initializer>(type, args);
  } else {
    return LoadInputVar(type, "singleinput");
  }
}

Stmt* ShaderPrepPass::StoreOutputVar(Type* type, std::string name, Expr* value) {
  auto var = std::make_shared<Var>(name, type);
  outputs_.push_back(var);
  outputIndices_.push_back(-1);
  return Make<StoreStmt>(Make<VarExpr>(var.get()), value);
}

// Given a type, adds corresponding Output vars. For class types, do so for
// each of its fields.
void ShaderPrepPass::StoreOutputVars(Type* type, Expr* value, Stmts* stmts) {
  if (type->IsVoid()) {
    stmts->Append(Make<ExprStmt>(value));
  } else if (type->IsClass()) {
    auto var = std::make_shared<Var>("temp", type);
    stmts->AppendVar(var);
    auto varExpr = Make<VarExpr>(var.get());
    stmts->Append(Make<StoreStmt>(varExpr, value));
    auto classType = static_cast<ClassType*>(type);
    for (auto& field : classType->GetFields()) {
      Expr* fieldValue = Make<FieldAccess>(varExpr, field.get());
      fieldValue = Make<LoadExpr>(fieldValue);
      stmts->Append(StoreOutputVar(field->type, field->name, fieldValue));
    }
  } else {
    stmts->Append(StoreOutputVar(type, "singleoutput", value));
  }
}

Method* ShaderPrepPass::Run(Method* entryPoint) {
  shaderType_ = entryPoint->shaderType;
  const auto& formalArgList = entryPoint->formalArgList;
  assert(formalArgList.size() >= 2 && formalArgList.size() <= 3);
  entryPoint = PrepMethod(entryPoint);

  if (!(entryPoint->modifiers & Method::STATIC)) { ExtractPipelineVars(entryPoint->classType); }

  entryPointWrapper_ = std::make_unique<Method>(entryPoint->modifiers, types_->GetVoid(), "main",
                                                entryPoint->classType);
  entryPointWrapper_->workgroupSize = entryPoint->workgroupSize;
  auto newArgs = Make<ExprList>();
  ExtractBuiltInVars(formalArgList[1]->type);
  entryPoint->formalArgList.clear();
  if (formalArgList.size() > 2) {
    Expr* input = LoadInputVars(formalArgList[2]->type);
    entryPoint->formalArgList.push_back(formalArgList[2]);
    newArgs->Append(input);
  }
  auto  stmts = Make<Stmts>();
  Expr* methodCall = Make<MethodCall>(entryPoint, newArgs);
  StoreOutputVars(entryPoint->returnType, methodCall, stmts);
  stmts->Append(Make<ReturnStatement>(nullptr, nullptr));
  entryPointWrapper_->stmts = stmts;
  return entryPointWrapper_.get();
}

Method* ShaderPrepPass::PrepMethod(Method* method) {
  if (method->classType->IsNative()) { return method; }

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

Type* ShaderPrepPass::GetWrapper(Type* type, int qualifiers) {
  if (Type* result = wrapper_[type]) { return result; }

  std::string name = "{" + type->ToString() + "}";
  ClassType* wrapper = types_->Make<ClassType>(name);
  wrapper->AddField("_", type, nullptr);
  Type* result = types_->GetQualifiedType(wrapper, qualifiers);
  return wrapper_[type] = result;
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
