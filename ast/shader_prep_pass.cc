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

namespace {

// If true, this type can be used for formal parameters and local variables.
// If false, this type most be resolved to a global during this pass.
bool IsValidLocalVar(Type* type) {
  if (type->IsPtr()) {
    type = static_cast<PtrType*>(type)->GetBaseType();
    int qualifiers;
    type = type->GetUnqualifiedType(&qualifiers);
    if (type->IsArray()) {
      type = static_cast<ArrayType*>(type)->GetElementType();
      type = type->GetUnqualifiedType(&qualifiers);
    }
    if (type->IsClass() && static_cast<ClassType*>(type)->HasNativeMethods()) {
      return false;
    }
    if (qualifiers & (Type::Qualifier::Uniform | Type::Qualifier::Storage)) {
      return false;
    }
  }
  return true;
}

bool NeedsUnfolding(Type* type) {
  if (type->IsPtr()) { return NeedsUnfolding(static_cast<PtrType*>(type)->GetBaseType()); }
  type = type->GetUnqualifiedType();
  if (type->IsClass()) {
    auto classType = static_cast<ClassType*>(type);
    // If this is a non-native class with no fields, unfold it into nothing.
    if (classType->GetTotalFields() == 0 && !classType->HasNativeMethods()) {
      return true;
    }
    if (classType->GetParent() && NeedsUnfolding(classType->GetParent())) {
      return true;
    }
    if (classType->GetTemplate() == NativeClass::BindGroup) {
      return true;
    }
    for (const auto& field : classType->GetFields()) {
      if (!IsValidLocalVar(field->type)) {
        return true;
      } else if (NeedsUnfolding(field->type)) {
        return true;
      }
    }
    return false;
  }
  return false;
}

}

ShaderPrepPass::ShaderPrepPass(NodeVector* nodes, TypeTable* types)
    : CopyVisitor(nodes), types_(types) {}

Expr* ShaderPrepPass::ResolveVar(Var* var) {
  if (auto alias = varAliases_[var]) {
    return alias;
  }
  return Make<VarExpr>(var);
}

Result ShaderPrepPass::Visit(CastExpr* node) {
  if (node->GetType()->IsPtr()) {
    return Resolve(node->GetExpr());
  }
  return CopyVisitor::Visit(node);
}

Result ShaderPrepPass::Visit(FieldAccess* node) {
  auto base = Resolve(node->GetExpr());
  if (NeedsUnfolding(base->GetType(types_))) {
    if (base->IsTempVarExpr()) {
      base = static_cast<TempVarExpr*>(base)->GetInitExpr();
    }
    assert(base->IsVarExpr());
    Var* baseVar = static_cast<VarExpr*>(base)->GetVar();
    if (baseVar->type->IsRawPtr()) {
      baseVar = unfoldedPtrs_[baseVar].get();
    }
    int index = node->GetField()->index;
    return ResolveVar(unfoldedVars_[baseVar][index].get());
  }
  return CopyVisitor::Visit(node);
}

Result ShaderPrepPass::Visit(LoadExpr* node) {
  Type* type = node->GetType(types_);
  if (NeedsUnfolding(type)) {
    auto expr = Resolve(node->GetExpr());
    assert(expr->IsVarExpr());
    auto var = static_cast<VarExpr*>(expr)->GetVar();
    var = unfoldedPtrs_[var].get();
    return Make<VarExpr>(var);
  }
  if (type->IsPtr()) {
    return Resolve(node->GetExpr());
  }
  return CopyVisitor::Visit(node);
}

void ShaderPrepPass::UnfoldClass(ClassType* classType, VarVector* vars, VarVector* localVars, VarVector* globalVars) {
  if (classType->GetTemplate() == NativeClass::BindGroup) {
    auto arg = classType->GetTemplateArgs()[0];
    assert(arg->IsClass());
    classType = static_cast<ClassType*>(arg);
  }
  if (classType->GetParent()) {
    UnfoldClass(classType->GetParent(), vars, localVars, globalVars);
  }
  for (const auto& field : classType->GetFields()) {
    auto var = std::make_shared<Var>(field->name, field->type);
    vars->push_back(var);
    UnfoldVar(var, localVars, globalVars);
  }
}

void ShaderPrepPass::UnfoldVar(std::shared_ptr<Var> var, VarVector* localVars, VarVector* globalVars) {
  if (NeedsUnfolding(var->type)) {
    Type* type = var->type;
    if (type->IsPtr()) {
      auto baseType = static_cast<PtrType*>(type)->GetBaseType();
      auto baseVar = std::make_shared<Var>(var->name, baseType);
      UnfoldVar(baseVar, localVars, globalVars);
      unfoldedPtrs_[var.get()] = baseVar;
    } else if (type->IsClass()) {
      VarVector vars;
      UnfoldClass(static_cast<ClassType*>(type), &vars, localVars, globalVars);
      unfoldedVars_[var.get()] = vars;
    } else {
      assert(false);
    }
  } else if (IsValidLocalVar(var->type)) {
    localVars->push_back(var);
  } else  {
    globalVars->push_back(var);
  }
}

Result ShaderPrepPass::Visit(Stmts* stmts) {
  Stmts* newStmts = Make<Stmts>();

  if (!rootStmts_) { rootStmts_ = newStmts; }

  for (auto var : stmts->GetVars()) {
    VarVector localVars, globalVars;
    UnfoldVar(var, &localVars, &globalVars);
    // FIXME: allow Stmts to return a modifiable VarVector?
    for (auto subVar : localVars) {
      assert(!subVar->type->IsPtr());
      rootStmts_->AppendVar(subVar);
    }
  }

  for (Stmt* const& it : stmts->GetStmts()) {
    Stmt* stmt = Resolve(it);
    if (stmt) newStmts->Append(stmt);
  }
  return newStmts;
}

Result ShaderPrepPass::Visit(StoreStmt* node) {
  auto lhs = Resolve(node->GetLHS());
  auto rhs = Resolve(node->GetRHS());
  if (rhs->GetType(types_)->IsPtr()) {
    assert(lhs->IsVarExpr());
    auto lhsVar = static_cast<VarExpr*>(lhs)->GetVar();
    varAliases_[lhsVar] = rhs;
    return nullptr;
  } else {
    return Make<StoreStmt>(lhs, rhs);
  }
}

Result ShaderPrepPass::Visit(VarExpr* node) {
  return ResolveVar(node->GetVar());
}

Type* ShaderPrepPass::ConvertType(Type* type) {
  if (type->IsPtr()) {
    int qualifiers;
    type = static_cast<PtrType*>(type)->GetBaseType();
    type = type->GetUnqualifiedType(&qualifiers);
    if (type->IsClass()) {
      auto classType = static_cast<ClassType*>(type);
      if (classType->GetTemplate() == NativeClass::VertexInput) {
        assert(classType->GetTemplateArgs().size() == 1);
        return classType->GetTemplateArgs()[0];
      } else if (classType->GetTemplate() == NativeClass::Buffer) {
        assert(classType->GetTemplateArgs().size() == 1);
        type = classType->GetTemplateArgs()[0];
        if (qualifiers & (Type::Qualifier::Storage | Type::Qualifier::Uniform)) {
          if (!type->IsClass()) {
            type = GetWrapper(type, qualifiers);
          } else {
            type = types_->GetQualifiedType(type, qualifiers);
          }
          return type;
        } else if (qualifiers & Type::Qualifier::Index) {
          return nullptr;
        }
      } else if (classType->GetTemplate() == NativeClass::ColorAttachment) {
        type = classType->GetTemplateArgs()[0];
        type = static_cast<ClassType*>(type)->FindType("DeviceType");
        return types_->GetVector(type, 4);
      } else if (classType->GetTemplate() == NativeClass::BindGroup) {
        return classType->GetTemplateArgs()[0];
      }
    }
  }
  return type;
}

void ShaderPrepPass::ExtractPipelineVars(ClassType* classType, std::vector<Var*>* globalVars) {
  if (classType->GetParent()) { ExtractPipelineVars(classType->GetParent(), globalVars); }
  for (const auto& field : classType->GetFields()) {
    Type* type = field->type;

    assert(type->IsPtr());
    type = static_cast<PtrType*>(type)->GetBaseType();
    int   qualifiers;
    Type* unqualifiedType = type->GetUnqualifiedType(&qualifiers);
    assert(unqualifiedType->IsClass());
    ClassType* classType = static_cast<ClassType*>(unqualifiedType);
    if (classType->GetTemplate() == NativeClass::ColorAttachment) {
      if (methodModifiers_ & Method::Modifier::Fragment) {
        auto output = std::make_shared<Var>(field->name, ConvertType(field->type));
        outputs_.push_back(output);
        globalVars->push_back(output.get());
      } else {
        globalVars->push_back(nullptr);
      }
    } else if (classType->GetTemplate() == NativeClass::DepthStencilAttachment) {
      // Depth/stencil variables are inaccessible from device code.
      globalVars->push_back(nullptr);
    } else if (classType->GetTemplate() == NativeClass::VertexInput) {
      if (methodModifiers_ & Method::Modifier::Vertex) {
        auto input = std::make_shared<Var>(field->name, ConvertType(field->type));
        inputs_.push_back(input);
        globalVars->push_back(input.get());
      } else {
        globalVars->push_back(nullptr);
      }
    } else if (classType->GetTemplate() == NativeClass::Buffer &&
               qualifiers & Type::Qualifier::Index) {
      // Index buffers are inaccessible from device code.
      globalVars->push_back(nullptr);
    } else if (classType->GetTemplate() == NativeClass::BindGroup) {
      Type*     argType = classType->GetTemplateArgs()[0];
      VarVector bindGroup;
      if (argType->IsClass()) {
        auto* bindGroupClass = static_cast<ClassType*>(argType);
        for (auto& subField : bindGroupClass->GetFields()) {
          auto  var = std::make_shared<Var>(subField->name, ConvertType(subField->type));
          bindGroup.push_back(var);
          globalVars->push_back(var.get());
        }
      } else {
        auto  var = std::make_shared<Var>(field->name, ConvertType(argType));
        bindGroup.push_back(var);
        globalVars->push_back(var.get());
      }
      bindGroups_.push_back(bindGroup);
    } else {
      assert(false);
    }
  }
}

Expr* ShaderPrepPass::ExtractBuiltInVars(Type* type, Stmts* stmts, Stmts* postStmts) {
  assert(type->IsPtr());
  type = static_cast<PtrType*>(type)->GetBaseType();
  assert(type->IsClass());
  auto classType = static_cast<ClassType*>(type);
  auto localVar = std::make_shared<Var>("builtins", type);
  stmts->AppendVar(localVar);
  auto localVarExpr = Make<VarExpr>(localVar.get());
  for (const auto& field : classType->GetFields()) {
    auto var = std::make_shared<Var>(field->name, field->type);
    builtInVars_.push_back(var);
    auto fieldAccess = Make<FieldAccess>(localVarExpr, field.get());
    auto varExpr = Make<VarExpr>(var.get());
    if (field->type->IsReadable()) {
      auto value = Make<LoadExpr>(varExpr);
      stmts->Append(Make<StoreStmt>(fieldAccess, value));
    }
    if (field->type->IsWriteable()) {
      auto value = Make<LoadExpr>(fieldAccess);
      postStmts->Append(Make<StoreStmt>(varExpr, value));
    }
  }
  return localVarExpr;
}

Expr* ShaderPrepPass::CreateAndLoadInputVar(Type* type, std::string name) {
  auto var = std::make_shared<Var>(name, type);
  inputs_.push_back(var);
  return Make<LoadExpr>(Make<VarExpr>(var.get()));
}

// Given a class, creates Vars for each of its fields.
// Loads the globals and constructs an instance of the class via list Initializer.
Expr* ShaderPrepPass::CreateAndLoadInputVars(Type* type) {
  if (type->IsClass()) {
    auto classType = static_cast<ClassType*>(type);
    auto args = Make<ExprList>();
    for (auto& i : classType->GetFields()) {
      Field* field = i.get();
      args->Append(CreateAndLoadInputVar(field->type, field->name));
    }
    return Make<Initializer>(type, args);
  } else {
    return CreateAndLoadInputVar(type, "singleinput");
  }
}

Stmt* ShaderPrepPass::CreateAndStoreOutputVar(Type* type, std::string name, Expr* value) {
  auto var = std::make_shared<Var>(name, type);
  outputs_.push_back(var);
  return Make<StoreStmt>(Make<VarExpr>(var.get()), value);
}

// Given a type, adds corresponding Output vars. For class types, do so for
// each of its fields.
void ShaderPrepPass::CreateAndStoreOutputVars(Type* type, Expr* value, Stmts* stmts) {
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
      stmts->Append(CreateAndStoreOutputVar(field->type, field->name, fieldValue));
    }
  } else {
    stmts->Append(CreateAndStoreOutputVar(type, "singleoutput", value));
  }
}

Method* ShaderPrepPass::Run(Method* entryPoint) {
  methodModifiers_ = entryPoint->modifiers;
  const auto& formalArgList = entryPoint->formalArgList;

  int argIndex = entryPoint->modifiers & Method::Modifier::Static ? 0 : 1;
  auto builtins = formalArgList[argIndex];
  auto inputs = formalArgList.size() > 2 ? formalArgList[argIndex + 1] : nullptr;

  std::vector<Var*> globalVars;
  if (!(entryPoint->modifiers & Method::Modifier::Static)) {
    ExtractPipelineVars(entryPoint->classType, &globalVars);
  }

  auto stmts = Make<Stmts>();
  auto postStmts = Make<Stmts>();
  auto newArgs = Make<ExprList>();
  newArgs->Append(ExtractBuiltInVars(builtins->type, stmts, postStmts));

  entryPoint = PrepMethod(entryPoint, globalVars);

  entryPointWrapper_ = std::make_unique<Method>(entryPoint->modifiers, types_->GetVoid(), "main",
                                                entryPoint->classType);
  entryPointWrapper_->workgroupSize = entryPoint->workgroupSize;
  if (inputs) {
    Expr* input = CreateAndLoadInputVars(inputs->type);
    newArgs->Append(input);
  }

  Expr* methodCall = Make<MethodCall>(entryPoint, newArgs);
  CreateAndStoreOutputVars(entryPoint->returnType, methodCall, stmts);
  stmts->Append(postStmts);
  stmts->Append(Make<ReturnStatement>(nullptr));
  entryPointWrapper_->stmts = stmts;
  return entryPointWrapper_.get();
}

Method* ShaderPrepPass::PrepMethod(Method* method, std::vector<Var*> globalVars) {
  if (method->IsNative()) { return method; }

  MethodKey key{method, globalVars};
  if (methodMap_[key]) { return methodMap_[key].get(); }

  auto    newMethod = std::make_unique<Method>(*method);

  newMethod->formalArgList.clear();
  VarVector aliasVars;
  for (auto formalArg : method->formalArgList) {
    UnfoldVar(formalArg, &newMethod->formalArgList, &aliasVars);
  }

  assert(aliasVars.size() == globalVars.size());
  auto aliasVar = aliasVars.begin();
  for (auto globalVar : globalVars) {
    Expr* globalExpr = nullptr;
    if (globalVar) {
      globalExpr = Make<VarExpr>(globalVar);
      if (IsWrapper(globalVar->type)) {
        Type* type = globalVar->type->GetUnqualifiedType();
        assert(type->IsClass());
        ClassType* classType = static_cast<ClassType*>(type);
        globalExpr = Make<FieldAccess>(globalExpr, classType->GetFields()[0].get());
      }
    }
    varAliases_[aliasVar->get()] = globalExpr;
    aliasVar++;
  }

  Method* result = newMethod.get();
  methodMap_[key] = std::move(newMethod);
  Stmts* prevRootStmts = rootStmts_;
  rootStmts_ = nullptr;

  // Clear the node cache to avoid returning nodes from previous instantiations of this method.
  nodeCache_.clear();

  result->stmts = Resolve(method->stmts);
  rootStmts_ = prevRootStmts;

  return result;
}

Result ShaderPrepPass::ResolveNativeMethodCall(MethodCall* node) {
  Method*    method = node->GetMethod();
  auto       args = node->GetArgList()->Get();
  ClassType* classType = method->classType;
  if (classType->GetTemplate() == NativeClass::ColorAttachment && method->name == "Set") {
    auto store = Make<StoreStmt>(Resolve(args[0]), Resolve(args[1]));
    return Make<ExprWithStmt>(nullptr, store);
  } else if (classType->GetTemplate() == NativeClass::VertexInput ||
             (classType->GetTemplate() == NativeClass::Buffer && method->name == "Get")) {
    return Make<LoadExpr>(Resolve(args[0]));
  } else if (classType->GetTemplate() == NativeClass::Buffer && (method->name == "Map" || method->name == "MapRead" || method->name == "MapWrite")) {
    return Resolve(args[0]);
  } else if (classType->GetTemplate() == NativeClass::BindGroup && method->name == "Get") {
    return Resolve(args[0]);
  }
  return CopyVisitor::Visit(node);
}

Result ShaderPrepPass::Visit(MethodCall* node) {
  if (node->GetMethod()->IsNative()) { return ResolveNativeMethodCall(node); }
  const std::vector<Expr*>& args = node->GetArgList()->Get();
  auto*                     newArgs = Make<ExprList>();
  Stmts*                    writeStmts = nullptr;
  std::vector<Var*>         globalVars;
  auto                      formalArg = node->GetMethod()->formalArgList.begin();
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
    if (IsValidLocalVar((*formalArg)->type)) {
      newArgs->Append(arg);
    } else {
      assert(arg->IsVarExpr());
      Var* var = static_cast<VarExpr*>(arg)->GetVar();
      globalVars.push_back(var);
    }
    formalArg++;
  }
  Method*                   method = PrepMethod(node->GetMethod(), globalVars);
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
  wrappers_.insert(result);
  return wrapper_[type] = result;
}

bool ShaderPrepPass::IsWrapper(Type* type) const {
  return wrappers_.find(type) != wrappers_.end();
}

Result ShaderPrepPass::Visit(SmartToRawPtr* node) {
  // All pointers are raw pointers in SPIR-V.
  return Resolve(node->GetExpr());
}

Result ShaderPrepPass::Visit(ZeroInitStmt* node) {
  // All variables are zero-initialized already
  return nullptr;
}

Result ShaderPrepPass::Visit(ToRawArray* node) {
  // Array pointers in SPIR-V are just data.
  return Resolve(node->GetData());
}

Result ShaderPrepPass::Default(ASTNode* node) {
  assert(!"unsupported node type in ShaderPrepPass");
  return nullptr;
}

};  // namespace Toucan
