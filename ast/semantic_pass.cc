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

#include "semantic_pass.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include <filesystem>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <ranges>

#include "api_validator.h"

namespace Toucan {

namespace {

class UnresolvedClassVisitor : public Visitor {
 public:
  UnresolvedClassVisitor(SemanticPass* semanticPass) : semanticPass_(semanticPass) {}

 private:
  Result Visit(UnresolvedClassDefinition* node) override {
    semanticPass_->PreVisit(node);
    return {};
  }

  Result Visit(Stmts* stmts) override {
    for (auto stmt : stmts->GetStmts()) stmt->Accept(this);
    return {};
  }

  Result Default(ASTNode* node) override { return {}; }

  SemanticPass* semanticPass_;
};

// Returns in the index corresponding to a given swizzle char, or -1 if invalid.
int ParseSwizzleChar(char c) {
  if (c == 'x' || c == 'r' || c == 's') return 0;
  if (c == 'y' || c == 'g' || c == 't') return 1;
  if (c == 'z' || c == 'b' || c == 'p') return 2;
  if (c == 'w' || c == 'a' || c == 'q') return 3;
  return -1;
}

// Returns the passed-in length if successful, or the first invalid char.
size_t ParseSwizzle(const std::string& str, size_t baseLength, int* result) {
  size_t i = 0;
  for (char c : str) {
    size_t r = ParseSwizzleChar(c);
    if (r < 0 || r >= baseLength) {
      return i;
    }
    result[i++] = r;
  }
  return i;
}

bool HasDuplicates(const std::vector<int>& indices) {
  for (int i = 0; i < indices.size(); ++i) {
    for (int j = i + 1; j < indices.size(); ++j) {
      if (indices[i] == indices[j]) return true;
    }
  }
  return false;
}

}

SemanticPass::SemanticPass(NodeVector* nodes, TypeTable* types)
    : CopyVisitor(nodes), types_(types), numErrors_(0) {}

Stmts* SemanticPass::Run(Stmts* stmts) {
  UnresolvedClassVisitor ucv(this);
  stmts->Accept(&ucv);

  stmts = Resolve(stmts);

  APIValidator apiValidator;
  for (auto pair : typesToValidate_) {
    apiValidator.ValidateType(pair.type, pair.location);
  }
  numErrors_ += apiValidator.GetNumErrors();
  return stmts;
}

Result SemanticPass::Visit(SmartToRawPtr* node) {
  Expr* expr = Resolve(node->GetExpr());
  if (!expr) return nullptr;
  Type* exprType = expr->GetType(types_);
  if (!exprType->IsStrongPtr() && !exprType->IsWeakPtr()) {
    return Error("attempt to dereference a non-pointer");
  }
  return Make<SmartToRawPtr>(expr);
}

Result SemanticPass::Visit(ArrayAccess* node) {
  if (!node->GetIndex()) { return Error("variable-sized array type used as expression"); }
  Expr* expr = Resolve(node->GetExpr());
  if (!expr) return nullptr;
  Expr* index = Resolve(node->GetIndex());
  if (!index) return nullptr;

  expr = MakeIndexable(expr);
  if (!expr) {
    return Error("expression is not of indexable type");
  }

  return Make<ArrayAccess>(expr, index);
}

Result SemanticPass::Visit(SliceExpr* node) {
  auto expr = Resolve(node->GetExpr());
  if (!expr) return nullptr;
  auto start = Resolve(node->GetStart());
  auto end = Resolve(node->GetEnd());

  expr = MakeIndexable(expr);
  if (!expr) {
    return Error("expression is not of indexable type");
  }

  return Make<SliceExpr>(expr, start, end);
}

Result SemanticPass::Visit(CastExpr* node) {
  Expr* expr = Resolve(node->GetExpr());
  if (!expr) { return nullptr; }

  Type* srcType = expr->GetType(types_);
  Type* dstType = node->GetType();
  if (srcType == dstType) {
    return expr;
  } else if (srcType->CanWidenTo(dstType) || srcType->CanNarrowTo(dstType)) {
    return Widen(expr, dstType);
  } else {
    return Error("cannot cast value of type %s to %s", srcType->ToString().c_str(), dstType->ToString().c_str());
  }
}

Result SemanticPass::Visit(Data* node) { return node; }

Result SemanticPass::Visit(Decls* node) {
  Stmts* stmts = Make<Stmts>();
  for (auto decl : node->Get()) {
    if ((decl = Resolve(decl))) stmts->Append(decl);
  }
  return stmts;
}

Result SemanticPass::Visit(Stmts* stmts) {
  Stmts* newStmts = Make<Stmts>();
  symbols_.PushScope(stmts);
  for (auto stmt : stmts->GetStmts()) {
    if ((stmt = Resolve(stmt))) newStmts->Append(stmt);
  }
  symbols_.PopScope();
  for (auto var : stmts->GetVars()) newStmts->AppendVar(var);
  // Append destructor calls for any vars that need it.
  for (auto var : std::views::reverse(stmts->GetVars())) {
    if (var->type->NeedsDestruction() && !stmts->ContainsReturn()) {
      newStmts->Append(Make<DestroyStmt>(Make<VarExpr>(var.get())));
    }
  }
  return newStmts;
}

Result SemanticPass::Visit(ArgList* node) {
  auto* result = Make<ArgList>();
  for (auto arg : node->GetArgs()) {
    if (node->IsNamed()) {
      if (arg->GetID().empty()) {
        return Error("if one argument is named, all arguments must be named");
      }
    } else {
      if (!arg->GetID().empty()) {
        return Error("if one argument is unnamed, all arguments must be unnamed");
      }
    }
    auto value = Resolve(arg);
    if (!value) return nullptr;

    if (value->IsUnfold()) {
      auto type = value->GetExpr()->GetType(types_);
      if (!type->IsVector()) {
        return Error("non-vector argument to @ operator");
      }
      auto len = static_cast<VectorType*>(type)->GetNumElements();
      for (int i = 0; i < len; ++i) {
        result->Append(Make<Arg>("", Make<ExtractElementExpr>(value->GetExpr(), i)));
      }
    } else {
      result->Append(value);
    }
  }
  return result;
}

Result SemanticPass::Visit(UnresolvedInitializer* node) {
  Type*              type = node->GetType();
  ArgList*           argList = Resolve(node->GetArgList());
  if (!argList) return nullptr;

  auto               args = argList->GetArgs();
  std::vector<Expr*> exprs;
  if (type->ContainsRawPtr()) { return Error("cannot allocate a type containing a raw pointer"); }
  typesToValidate_.push_back({type, node->GetFileLocation()});
  if (type->IsClass() && node->IsConstructor()) {
    ClassType*         classType = static_cast<ClassType*>(type);
    TypeList           types;
    std::vector<Expr*> constructorArgs;
    Method* constructor = FindMethod(nullptr, classType, classType->GetName(), argList, &constructorArgs);
    if (!constructor) {
      return Error("constructor for class \"%s\" with those arguments not found",
                   classType->GetName().c_str());
    }
    constructorArgs[0] = Make<TempVarExpr>(type);
    WidenArgList(constructorArgs, constructor->formalArgList);
    auto* exprList = Make<ExprList>(std::move(constructorArgs));
    Expr* result = Make<MethodCall>(constructor, exprList);
    return Make<LoadExpr>(result);
  }
  return ResolveListExpr(argList, type);
}

Stmt* SemanticPass::Initialize(Expr* dest, Expr* initExpr) {
  auto type = dest->GetType(types_);
  assert(type->IsRawPtr());
  type = static_cast<RawPtrType*>(type)->GetBaseType();
  if (initExpr) {
    Type* initExprType = initExpr->GetType(types_);
    if (!initExprType->CanWidenTo(type)) {
      WideningError(initExprType, type);
      return nullptr;
    }
    initExpr = Widen(initExpr, type);
    return Make<StoreStmt>(dest, initExpr);
  } else if (type->IsClass()) {
    return InitializeClass(dest, static_cast<ClassType*>(type));
  } else if (type->IsArray()) {
    auto arrayType = static_cast<ArrayType*>(type);
    return InitializeArray(MakeIndexable(dest), arrayType->GetElementType(), Make<IntConstant>(arrayType->GetNumElements(), 32), Make<ExprList>());
  } else {
    return Make<ZeroInitStmt>(dest);
  }
}

Stmts* SemanticPass::InitializeClass(Expr* dest, ClassType* classType) {
  Stmts* stmts = Make<Stmts>();
  if (classType->GetParent()) { stmts->Append(InitializeClass(dest, classType->GetParent())); }
  for (const auto& field : classType->GetFields()) {
    Expr* fieldExpr = Make<FieldAccess>(dest, field.get());
    stmts->Append(Initialize(fieldExpr, field->defaultValue));
  }
  return stmts;
}

Stmts* SemanticPass::InitializeArray(Expr* dest, Type* elementType, Expr* length, ExprList* exprList) {
  auto stmts = Make<Stmts>();
  auto indexVar = std::make_shared<Var>("", types_->GetInt());
  stmts->AppendVar(indexVar);
  auto index = Make<VarExpr>(indexVar.get());
  auto lhs = Make<ArrayAccess>(dest, Make<LoadExpr>(index));
  auto initStmt = Make<StoreStmt>(index, Make<IntConstant>(0, 32));
  auto cond = Make<BinOpNode>(BinOpNode::Op::LT, Make<LoadExpr>(index), length);
  auto indexPlusOne = Make<BinOpNode>(BinOpNode::Op::ADD, Make<LoadExpr>(index), MakeConstantOne(types_->GetInt()));
  auto loopStmt = Make<StoreStmt>(index, indexPlusOne);
  Stmt* body;
  int numArgs = exprList->Get().size();
  if (numArgs == 0) {
    body = Initialize(lhs);
  } else if (numArgs == 1) {
    body = Make<StoreStmt>(lhs, exprList->Get()[0]);
  } else {
    Type* type = types_->GetArrayType(elementType, numArgs, MemoryLayout::Default);
    auto value = Make<TempVarExpr>(type, Make<Initializer>(type, exprList));
    auto rhs = Make<LoadExpr>(Make<ArrayAccess>(MakeIndexable(value), Make<LoadExpr>(index)));
    body = Make<StoreStmt>(lhs, rhs);
  }
  stmts->Append(Make<ForStatement>(initStmt, cond, loopStmt, body));
  return stmts;
}

Result SemanticPass::Visit(VarDeclaration* decl) {
  std::string id = decl->GetID();
  Type*       type = decl->GetType();
  if (symbols_.PeekScope()->FindID(id)) {
    return Error("identifier \"%s\" already defined in this scope", id.c_str());
  }
  if (!type) return nullptr;
  Expr* initExpr = nullptr;
  if (decl->GetInitExpr()) {
    initExpr = Resolve(decl->GetInitExpr());
    if (!initExpr) { return nullptr; }
  }
  if (type->IsAuto()) {
    assert(initExpr);
    type = initExpr->GetType(types_);
  }
  typesToValidate_.push_back({type, decl->GetFileLocation()});
  if (type->IsVoid() || (type->IsArray() && static_cast<ArrayType*>(type)->GetNumElements() == 0)) {
    std::string errorMsg = std::string("cannot create storage of type ") + type->ToString();
    return Error(errorMsg.c_str());
  }
  if (type->IsRawPtr() && !initExpr) {
    return Error("reference must be initialized");
  }
  if (!type->IsRawPtr() && type->ContainsRawPtr()) {
    return Error("cannot allocate a type containing a raw pointer");
  }
  auto var = std::make_shared<Var>(id, type);
  symbols_.PeekScope()->AppendVar(var);
  Expr* varExpr = Make<VarExpr>(var.get());
  symbols_.DefineID(id, var->type->IsRawPtr() ? Make<LoadExpr>(varExpr) : varExpr);
  return Initialize(varExpr, initExpr);
}

Result SemanticPass::Visit(ConstDecl* decl) {
  std::string id = decl->GetID();
  if (symbols_.PeekScope()->FindID(id)) {
    return Error("identifier \"%s\" already defined in this scope", id.c_str());
  }
  auto expr = Resolve(decl->GetExpr());
  if (!expr) return nullptr;
  auto type = expr->GetType(types_);

  if (!expr->IsConstant(types_)) {
    return Error("expression is not constant");
  }
  typesToValidate_.push_back({type, decl->GetFileLocation()});

  // This will be removed by the load expression added by the parser to turn assignable to expr.
  expr = MakeReadOnlyTempVar(expr);

  symbols_.DefineID(id, expr);
  return {};
}

Result SemanticPass::ResolveMethodCall(Expr*       expr,
                                       ClassType*  classType,
                                       std::string id,
                                       ArgList*    arglist) {
  std::vector<Expr*> newArgList;
  Method*            method = FindMethod(expr, classType, id, arglist, &newArgList);
  if (!method) {
    std::string msg = "class " + classType->ToString() + " has no method " + id;
    msg += "(";
    for (auto arg : arglist->GetArgs()) {
      if (arg->GetID() != "") { msg += arg->GetID() + " = "; }
      msg += arg->GetExpr()->GetType(types_)->ToString();
      if (arg != arglist->GetArgs().back()) { msg += ", "; }
    }
    msg += ")";
    Error(msg.c_str());

    for (const auto& method : classType->GetMethods()) {
      if (method->name == id) { Error(method->ToString().c_str()); }
    }
    return nullptr;
  }
  if (!(method->modifiers & Method::Modifier::Static)) {
    if (!expr) {
      return Error("attempt to call non-static method \"%s\" on class \"%s\"", id.c_str(),
                   classType->ToString().c_str());
    } else {
      newArgList[0] = expr;
    }
  }
  for (int i = 0; i < newArgList.size(); ++i) {
    if (!newArgList[i]) {
      return Error("formal parameter \"%s\" has no default value",
                   method->formalArgList[i]->name.c_str());
    }
  }
  WidenArgList(newArgList, method->formalArgList);
  auto* exprList = Make<ExprList>(std::move(newArgList));
  Expr* result = Make<MethodCall>(method, exprList);
  if (!method->returnType->IsRawPtr()) {
    // All non-rawptr return values are turned into rawptr to enable chaining.
    return Make<TempVarExpr>(method->returnType, result);
  }
  return result;
}

Expr* SemanticPass::Widen(Expr* node, Type* dstType) {
  Type* srcType = node->GetType(types_);
  if (srcType->GetUnqualifiedType() == dstType->GetUnqualifiedType()) {
    return node;
  } else if (node->IsUnresolvedListExpr()) {
    return ResolveListExpr(static_cast<UnresolvedListExpr*>(node)->GetArgList(), dstType);
  } else if ((srcType->IsStrongPtr() || srcType->IsWeakPtr()) && dstType->IsRawPtr()) {
    return Make<SmartToRawPtr>(node);
  } else if (dstType->IsRawPtr() && static_cast<RawPtrType*>(dstType)->GetBaseType()->IsArray()) {
    return MakeIndexable(node);
  } else if (srcType->IsRawPtr() && dstType->IsRawPtr()) {
    return node;
  } else {
    return Make<CastExpr>(dstType, node);
  }
}

Expr* SemanticPass::MakeIndexable(Expr* expr) {
  expr = AutoDereference(expr);
  Type* type = expr->GetType(types_);
  assert(type->IsRawPtr());
  type = static_cast<RawPtrType*>(type)->GetBaseType();
  type = type->GetUnqualifiedType();
  if (type->IsUnsizedArray()) {
    return expr;
  }
  if (!type->IsArrayLike()) {
    return nullptr;
  }

  auto arrayLikeType = static_cast<ArrayLikeType*>(type);
  MemoryLayout memoryLayout = MemoryLayout::Default;
  if (type->IsArray()) {
    memoryLayout = static_cast<ArrayType*>(type)->GetMemoryLayout();
  }
  auto lengthExpr = Make<IntConstant>(arrayLikeType->GetNumElements(), 32);
  return Make<ToRawArray>(expr, lengthExpr, arrayLikeType->GetElementType(), memoryLayout);
}

Expr* SemanticPass::MakeDefaultInitializer(Type* type) {
  if (type->IsClass()) {
    auto classType = static_cast<ClassType*>(type);
    std::vector<Expr*> exprs(classType->GetTotalFields());
    AddDefaultInitializers(type, &exprs);
    auto exprList = Make<ExprList>(std::move(exprs));
    return Make<Initializer>(type, exprList);
  }
  return nullptr;
}

void SemanticPass::AddDefaultInitializers(Type* type, std::vector<Expr*>* exprs) {
  if (type->IsClass()) {
    auto  classType = static_cast<ClassType*>(type);
    for (ClassType* c = classType; c != nullptr; c = c->GetParent()) {
      for (auto& field : c->GetFields()) {
        if ((*exprs)[field->index] == nullptr) {
          if (field->defaultValue) {
            (*exprs)[field->index] = Widen(field->defaultValue, field->type);
          } else {
            (*exprs)[field->index] = MakeDefaultInitializer(field->type);
          }
        }
      }
    }
  }
}

Expr* SemanticPass::ResolveListExpr(ArgList* argList, Type* dstType) {
  if (dstType->IsRawPtr()) {
    auto baseType = static_cast<RawPtrType*>(dstType)->GetBaseType();
    if (baseType->IsUnsizedArray()) {
      // Resolve as &[N] of list length, then convert to &[]
      auto arrayType = static_cast<ArrayType*>(baseType);
      auto length = argList->GetArgs().size();
      dstType = types_->GetArrayType(arrayType->GetElementType(), length, arrayType->GetMemoryLayout());
      dstType = types_->GetRawPtrType(dstType);
      return MakeIndexable(ResolveListExpr(argList, dstType));
    }
    return Make<TempVarExpr>(baseType, ResolveListExpr(argList, baseType));
  }
  std::vector<Expr*> exprs;
  if (dstType->IsClass()) {
    auto  classType = static_cast<ClassType*>(dstType);
    if (argList->IsNamed() || argList->GetArgs().size() == 0) {
      exprs.resize(classType->GetTotalFields(), nullptr);
      for (auto arg : argList->GetArgs()) {
        Field* field = classType->FindField(arg->GetID());
        exprs[field->index] = Widen(arg->GetExpr(), field->type);
      }
      AddDefaultInitializers(classType, &exprs);
    } else {
      auto& fields = classType->GetFields();
      int i = 0;
      assert(argList->GetArgs().size() == classType->GetTotalFields());
      for (auto arg : argList->GetArgs()) {
        exprs.push_back(Widen(arg->GetExpr(), fields[i++]->type));
      }
    }
  } else {
    if (argList->IsNamed()) {
      Error("named list expression are unsupported for %s", dstType->ToString().c_str());
      return nullptr;
    }
    if (!dstType->IsArrayLike()) {
      assert(!"invalid type in list expression");
      return nullptr;
    }
    auto arrayLikeType = static_cast<ArrayLikeType*>(dstType);
    Type* elementType = arrayLikeType->GetElementType();
    uint32_t length = arrayLikeType->GetNumElements();
    if (argList->GetArgs().size() == 1) {
      Expr* arg = Widen(argList->GetArgs()[0]->GetExpr(), elementType);
      for (int i = 0; i < length; ++i) {
        exprs.push_back(arg);
      }
    } else {
      for (auto arg : argList->GetArgs()) {
        exprs.push_back(Widen(arg->GetExpr(), elementType));
      }
    }
  }
  auto exprList = Make<ExprList>(std::move(exprs));
  return Make<Initializer>(dstType, exprList);
}

Result SemanticPass::Visit(UnresolvedMethodCall* node) {
  std::string id = node->GetID();
  Expr*       expr = Resolve(node->GetExpr());
  if (!expr) return nullptr;
  ArgList* arglist = Resolve(node->GetArgList());
  if (!arglist) return nullptr;
  expr = AutoDereference(expr);
  Type* type = expr->GetType(types_);
  assert(type->IsRawPtr());
  type = static_cast<RawPtrType*>(type)->GetBaseType();
  type = type->GetUnqualifiedType();
  if (!type->IsClass()) { return Error("expression does not evaluate to class type"); }
  ClassType* classType = static_cast<ClassType*>(type);
  return ResolveMethodCall(expr, classType, id, arglist);
}

Result SemanticPass::Visit(UnresolvedStaticMethodCall* node) {
  std::string id = node->GetID();
  ArgList*    arglist = Resolve(node->GetArgList());
  if (!arglist) return nullptr;
  ClassType* classType = node->classType();
  return ResolveMethodCall(nullptr, classType, id, arglist);
}

Expr* SemanticPass::MakeLoad(Expr* expr) {
  assert(expr->GetType(types_)->IsRawPtr());
  if (expr->IsTempVarExpr()) {
    return static_cast<TempVarExpr*>(expr)->GetInitExpr();
  }
  return Make<LoadExpr>(expr);
}

Result SemanticPass::Visit(LoadExpr* node) {
  Expr* expr = Resolve(node->GetExpr());
  if (!expr) return nullptr;
  return MakeLoad(expr);
}

Result SemanticPass::Visit(UnresolvedIdentifier* node) {
  std::string id = node->GetID();
  if (Expr* expr = symbols_.FindID(id)) {
    copyFileLocation_ = false;
    expr = Resolve(expr);
    copyFileLocation_ = true;
    return expr;
  } else {
    return Error("unknown symbol \"%s\"", id.c_str());
  }
}

Result SemanticPass::MakeSwizzle(int srcLength, Expr* lhs, const std::string& swizzle) {
  auto indices = std::vector<int>(swizzle.size());
  size_t resultLength = ParseSwizzle(swizzle, srcLength, indices.data());
  if (resultLength < swizzle.size()) {
    return Error("invalid swizzle component '%c'", swizzle[resultLength]);
  }
  Expr* expr = MakeLoad(lhs);
  if (indices.size() == 1) {
    expr = Make<ExtractElementExpr>(expr, indices[0]);
  } else {
    expr = Make<SwizzleExpr>(expr, indices);
  }
  return Make<TempVarExpr>(expr->GetType(types_), expr);
}

Result SemanticPass::WideningError(Type* srcType, Type* dstType) {
  return Error("cannot store a value of type \"%s\" to a location of type \"%s\"",
               srcType->ToString().c_str(), dstType->ToString().c_str());
}

Expr* SemanticPass::MakeSwizzleForStore(VectorType* lhsType, Expr* lhs, const std::string& swizzle, Expr* rhs) {
  auto indices = std::vector<int>(swizzle.size());
  size_t resultLength = ParseSwizzle(swizzle, lhsType->GetNumElements(), indices.data());
  if (resultLength < swizzle.size()) {
    Error("invalid swizzle component '%c'", swizzle[resultLength]);
    return nullptr;
  }
  if (HasDuplicates(indices)) {
    Error("duplicate components in swizzle store");
    return nullptr;
  }
  Type* rhsType = rhs->GetType(types_);
  auto dstType = types_->GetVector(lhsType->GetElementType(), indices.size());
  if (indices.size() == 1) {
    if (!rhsType->CanWidenTo(lhsType->GetElementType())) {
      WideningError(rhsType, lhsType->GetElementType());
      return nullptr;
    }
    lhs = Make<InsertElementExpr>(lhs, rhs, indices[0]);
  } else {
    if (!rhsType->CanWidenTo(dstType)) {
      WideningError(rhsType, dstType);
      return nullptr;
    }
    for (int i = 0; i < indices.size(); ++i) {
      auto value = Make<ExtractElementExpr>(rhs, i);
      lhs = Make<InsertElementExpr>(lhs, value, indices[i]);
    }
  }
  return lhs;
}

Result SemanticPass::Visit(UnresolvedDot* node) {
  Expr* expr = Resolve(node->GetExpr());
  if (!expr) return nullptr;
  expr = AutoDereference(expr);
  Type* type = expr->GetType(types_);
  assert(type->IsRawPtr());
  type = static_cast<RawPtrType*>(type)->GetBaseType();
  std::string id = node->GetID();
  type = type->GetUnqualifiedType();
  if (type->IsArray()) {
    if (id == "length") {
      ArrayType* atype = static_cast<ArrayType*>(type);
      Expr* lengthExpr;
      if (atype->GetNumElements() > 0) {
        lengthExpr = Make<IntConstant>(atype->GetNumElements(), 32);
      } else {
        lengthExpr = Make<LengthExpr>(expr);
      }
      return MakeReadOnlyTempVar(lengthExpr);
    } else {
      return Error("unknown array property \"%s\"", id.c_str());
    }
  } else if (type->IsClass()) {
    ClassType* classType;
    classType = static_cast<ClassType*>(type);
    if (auto field = classType->FindField(id)) {
      return Make<FieldAccess>(expr, field);
    } else if (auto constant = classType->FindConstant(id)) {
      return MakeReadOnlyTempVar(constant);
    } else {
      return Error("field \"%s\" not found on class \"%s\"", id.c_str(),
                   classType->ToString().c_str());
    }
  } else if (type->IsVector()) {
    return MakeSwizzle(static_cast<VectorType*>(type)->GetNumElements(), expr, id);
  } else {
    return Error("Expression is not of class, reference or vector type");
  }
}

Result SemanticPass::Visit(UnresolvedStaticDot* node) {
  auto type = node->GetType();
  if (!type) return nullptr;
  std::string id = node->GetID();
  if (type->IsEnum()) {
    auto enumType = static_cast<EnumType*>(type);
    const EnumValue* enumValue = enumType->FindValue(id);
    if (enumValue) {
      return MakeReadOnlyTempVar(Make<EnumConstant>(enumValue));
    } else {
      return Error("value \"%s\" not found on enum \"%s\"", id.c_str(),
                   enumType->ToString().c_str());
    }
  } else if (type->IsClass()) {
    if (auto constant = static_cast<ClassType*>(type)->FindConstant(id)) {
      return MakeReadOnlyTempVar(constant);
    } else {
      return Error("identifier \"%s\" not found on class \"%s\"", id.c_str(),
                   type->ToString().c_str());
    }
  } else {
    return Error("expression is not of class or enum type");
  }
}

Expr* SemanticPass::MakeConstantOne(Type* type) {
  if (type->IsInteger()) {
    return Make<IntConstant>(1, static_cast<IntegerType*>(type)->GetBits());
  } else if (type->IsFloat()) {
    return Make<FloatConstant>(1.0f);
  } else if (type->IsDouble()) {
    return Make<DoubleConstant>(1.0);
  } else {
    assert(!"unexpected type for constant");
    return nullptr;
  }
}

Expr* SemanticPass::MakeReadOnlyTempVar(Expr* expr) {
  auto type = expr->GetType(types_);
  type = types_->GetQualifiedType(type, Type::Qualifier::ReadOnly);
  return Make<TempVarExpr>(type, expr);
}

Result SemanticPass::Visit(IncDecExpr* node) {
  Expr* expr = Resolve(node->GetExpr());
  if (!expr) return nullptr;
  Expr* value = Make<LoadExpr>(expr);
  Type* type = value->GetType(types_);
  auto op = node->GetOp() == IncDecExpr::Op::Inc ? BinOpNode::Op::ADD
                                                 : BinOpNode::Op::SUB;
  Expr* result = Make<BinOpNode>(op, value, MakeConstantOne(type));
  Stmt* store = Make<StoreStmt>(expr, result);
  return Make<ExprWithStmt>(node->returnOrigValue() ? value : result, store);
}

Result SemanticPass::Visit(ExprWithStmt* node) {
  Expr* expr = Resolve(node->GetExpr());
  if (!expr) return nullptr;
  Stmt* stmt = Resolve(node->GetStmt());
  return Make<ExprWithStmt>(expr, stmt);
}

Result SemanticPass::Visit(StoreStmt* node) {
  Expr* rhs = Resolve(node->GetRHS());
  if (!rhs) return nullptr;
  Expr* lhs = nullptr;
  if (node->GetLHS()->IsUnresolvedDot()) {
    auto dot = static_cast<UnresolvedDot*>(node->GetLHS());
    auto base = Resolve(dot->GetExpr());
    if (!base) return nullptr;
    base = AutoDereference(base);
    Expr* expr = MakeLoad(base);
    auto exprType = expr->GetType(types_);
    if (exprType->IsVector()) {
      rhs = MakeSwizzleForStore(static_cast<VectorType*>(exprType), expr, dot->GetID(), rhs);
      if (!rhs) return nullptr;
      lhs = base;
    }
  }
  if (!lhs) {
    lhs = Resolve(node->GetLHS());
    if (!lhs) return nullptr;
  }
  Type* lhsType = lhs->GetType(types_);
  if (!lhsType->IsRawPtr()) { return Error("expression is not an assignable value"); }
  lhsType = static_cast<RawPtrType*>(lhsType)->GetBaseType();
  Type* rhsType = rhs->GetType(types_);
  int   qualifiers;
  lhsType = lhsType->GetUnqualifiedType(&qualifiers);
  if (qualifiers & Type::Qualifier::ReadOnly) {
    return Error("expression is not an assignable value");
  } else if (!rhsType->CanWidenTo(lhsType)) {
    return WideningError(rhsType, lhsType);
  }
  rhs = Widen(rhs, lhsType);
  if (lhsType->NeedsDestruction()) {
    auto stmts = Make<Stmts>();
    stmts->Append(Make<DestroyStmt>(lhs));
    stmts->Append(Make<StoreStmt>(lhs, rhs));
    return stmts;
  }
  return Make<StoreStmt>(lhs, rhs);
}

namespace {

bool IsValidNonMatchingOp(BinOpNode::Op op, Type* lhs, Type* rhs) {
  if (op == BinOpNode::MUL) {
    return TypeTable::VectorScalar(lhs, rhs) || TypeTable::ScalarVector(lhs, rhs) ||
           TypeTable::MatrixScalar(lhs, rhs) || TypeTable::ScalarMatrix(lhs, rhs) ||
           TypeTable::MatrixVector(lhs, rhs) || TypeTable::VectorMatrix(lhs, rhs);
  } else if (op == BinOpNode::DIV) {
    return TypeTable::VectorScalar(lhs, rhs) || TypeTable::ScalarVector(lhs, rhs);
    TypeTable::MatrixScalar(lhs, rhs) || TypeTable::ScalarMatrix(lhs, rhs);
  } else {
    return false;
  }
}

}  // namespace

Result SemanticPass::Visit(BinOpNode* node) {
  Expr* rhs = Resolve(node->GetRHS());
  if (!rhs) return nullptr;
  Expr* lhs = Resolve(node->GetLHS());
  if (!lhs) return nullptr;
  Type* lhsType = lhs->GetType(types_);
  Type* rhsType = rhs->GetType(types_);
  bool  isEqualityOp = node->GetOp() == BinOpNode::EQ || node->GetOp() == BinOpNode::NE;
  if (!lhsType->CanWidenTo(rhsType) && !rhsType->CanWidenTo(lhsType) &&
      !IsValidNonMatchingOp(node->GetOp(), lhsType, rhsType)) {
    return Error("type mismatch on binary operator");
  }
  if (node->GetOp() == BinOpNode::LOGICAL_AND || node->GetOp() == BinOpNode::LOGICAL_AND) {
    if (!lhsType->IsBool() || !rhsType->IsBool()) {
      return Error("non-boolean argument to logical operator");
    }
  }
  if (node->IsRelOp()) {
    if (lhsType->IsEnum() || lhsType->IsBool() || lhsType->IsPtr() || lhsType->IsVector()) {
      if (!isEqualityOp) { return Error("invalid type for binary operator"); }
    } else if (!(lhsType->IsInt() || lhsType->IsUInt() || lhsType->IsFloatingPoint())) {
      return Error("invalid type for relational operator");
    }
  }
  if (lhsType != rhsType) {
    if (lhsType->CanWidenTo(rhsType)) {
      lhs = Widen(lhs, rhsType);
    } else if (rhsType->CanWidenTo(lhsType)) {
      rhs = Widen(rhs, lhsType);
    }
  }
  return Make<BinOpNode>(node->GetOp(), lhs, rhs);
}

void SemanticPass::WidenArgList(std::vector<Expr*>& argList, const VarVector& formalArgList) {
  assert(argList.size() == formalArgList.size());
  for (int i = 0; i < argList.size(); ++i) {
    if (!argList[i]) continue;
    Type* argType = argList[i]->GetType(types_);
    Type* expectedArgType = formalArgList[i]->type;
    argList[i] = Widen(argList[i], formalArgList[i]->type);
  }
}

Result SemanticPass::Visit(UnresolvedNewExpr* node) {
  Type* type = node->GetType();
  if (!type) return nullptr;
  typesToValidate_.push_back({type, node->GetFileLocation()});
  if (type->IsUnsizedArray()) { return Error("cannot allocate unsized array"); }
  if (type->ContainsRawPtr()) { return Error("cannot allocate a type containing raw pointer"); }

  ArgList* arglist = Resolve(node->GetArgList());
  if (!arglist) return nullptr;
  int       qualifiers;
  Type*     unqualifiedType = type->GetUnqualifiedType(&qualifiers);
  Expr*     length = node->GetLength() ? Resolve(node->GetLength()) : nullptr;
  auto      allocation = Make<HeapAllocation>(type, length);
  if (unqualifiedType->IsClass()) {
    auto* classType = static_cast<ClassType*>(unqualifiedType);
    if (classType->IsUnsizedClass()) {
      if (!length) { return Error("class with unsized array must be allocated with size"); }
    }
    if (!classType->IsFullySpecified()) {
      return Error("cannot allocate partially-specified template class %s",
                   classType->ToString().c_str());
    }
    if (node->IsConstructor()) {
      std::vector<Expr*> exprList;
      Method* constructor = FindMethod(nullptr, classType, classType->GetName(), arglist, &exprList);
      if (!constructor) {
        return Error("matching constructor not found");
      }
      for (int i = 1; i < exprList.size(); ++i) {
        if (!exprList[i]) {
          return Error("formal parameter \"%s\" has no default value",
                       constructor->formalArgList[i]->name.c_str());
        }
      }
      WidenArgList(exprList, constructor->formalArgList);
      exprList[0] = allocation;
      auto args = Make<ExprList>(std::move(exprList));
      Expr* result = Make<MethodCall>(constructor, args);
      result = Make<RawToSmartPtr>(result);
      // This is for native templated constructors, which return an untemplated type
      if (result->GetType(types_) != node->GetType(types_)) {
        result = Widen(result, node->GetType(types_));
      }
      return result;
    }
  }
  Stmt* stmt;
  if (length && !type->IsUnsizedClass()) {
    if (arglist->IsNamed()) {
      return Error("array initializer list must not be named");
    }
    auto exprList = Make<ExprList>();
    for (int i = 0; i < arglist->GetArgs().size(); ++i) {
      exprList->Append(arglist->GetArgs()[i]->GetExpr());
    }
    stmt = InitializeArray(allocation, type, length, exprList);
  } else {
    stmt = Initialize(allocation);
  }
  return Make<RawToSmartPtr>(Make<ExprWithStmt>(allocation, stmt));
}

Result SemanticPass::Visit(IfStatement* s) {
  Expr* expr = Resolve(s->GetExpr());
  if (expr && expr->GetType(types_) != types_->GetBool()) {
    return Error("condition must be boolean");
  } else {
    Stmt* stmt = Resolve(s->GetStmt());
    Stmt* optElse = Resolve(s->GetOptElse());
    return Make<IfStatement>(expr, stmt, optElse);
  }
}

Result SemanticPass::Visit(WhileStatement* s) {
  Expr* cond = Resolve(s->GetCond());
  Stmt* body = Resolve(s->GetBody());
  if (cond && cond->GetType(types_) != types_->GetBool()) {
    return Error("condition must be boolean");
  } else {
    return Make<WhileStatement>(cond, body);
  }
}

Result SemanticPass::Visit(DoStatement* s) {
  Stmt* body = Resolve(s->GetBody());
  Expr* cond = Resolve(s->GetCond());
  if (cond && cond->GetType(types_) != types_->GetBool()) {
    return Error("condition must be boolean");
  } else {
    return Make<DoStatement>(body, cond);
  }
}

Result SemanticPass::Visit(ForStatement* node) {
  Stmt* initStmt = Resolve(node->GetInitStmt());
  Expr* cond = Resolve(node->GetCond());
  Stmt* loopStmt = Resolve(node->GetLoopStmt());
  Stmt* body = Resolve(node->GetBody());
  return Make<ForStatement>(initStmt, cond, loopStmt, body);
}

void SemanticPass::PreVisit(UnresolvedClassDefinition* defn) {
  ClassType* classType = defn->GetClass();

  // Non-native template classes don't need inferred type resolution
  if (!classType->HasNativeMethods() && classType->IsClassTemplate()) return;

  for (const auto& method : classType->GetMethods()) {
    for (int i = 0; i < method->defaultArgs.size(); ++i) {
      method->defaultArgs[i] = Resolve(method->defaultArgs[i]);
      if (method->formalArgList[i]->type->IsAuto()) {
        method->formalArgList[i]->type = method->defaultArgs[i]->GetType(types_);
      }
    }
  }

  for (const auto& field : classType->GetFields()) {
    field->defaultValue = Resolve(field->defaultValue);
    if (field->type->IsAuto()) {
      field->type = field->defaultValue->GetType(types_);
    }
  }
}

Result SemanticPass::Visit(UnresolvedClassDefinition* defn) {
  ClassType* classType = defn->GetClass();

  // Template classes don't need semantic analysis, since their code won't be directly generated.
  if (classType->IsClassTemplate()) return nullptr;

  symbols_.PushScope(Make<Stmts>());

  for (auto c = classType; c != nullptr; c = c->GetParent()) {
    for (const auto& field : c->GetFields()) {
      Expr* thisPtr = Make<UnresolvedIdentifier>("this");
      symbols_.DefineID(field->name, Make<FieldAccess>(thisPtr, field.get()));
    }
    for (const auto& constant : c->GetConstants()) {
      symbols_.DefineID(constant.first, MakeReadOnlyTempVar(constant.second));
    }
  }

  if (classType->NeedsDestruction()) {
    auto destructor = classType->GetDestructor();
    if (!destructor) {
      std::string name = std::string("~") + classType->GetName();
      destructor = new Method(0, types_->GetVoid(), name, classType);
      destructor->AddFormalArg("this", types_->GetRawPtrType(classType), nullptr);
      destructor->stmts = Make<Stmts>();
      classType->AddMethod(destructor);
    }

    if (destructor->stmts) {
      auto This = Make<LoadExpr>(Make<VarExpr>(destructor->formalArgList[0].get()));
      for (const auto& field : classType->GetFields()) {
        if (field->type->NeedsDestruction()) {
          destructor->stmts->Append(Make<DestroyStmt>(Make<FieldAccess>(This, field.get())));
        }
      }
    }
  }

  for (const auto& method : classType->GetMethods()) {
    if (!method->stmts) continue;

    symbols_.PushScope(Make<Stmts>());
    for (const auto& var : method->formalArgList) {
      Expr* expr = Make<VarExpr>(var.get());
      if (var->type->IsRawPtr()) expr = Make<LoadExpr>(expr);
      symbols_.DefineID(var->name, expr);
    }

    currentMethod_ = method.get();
    method->stmts = Resolve(method->stmts);
    currentMethod_ = nullptr;

    if (method->IsConstructor()) {
      Expr* initializer = method->initializer ? Resolve(method->initializer)
                          : ResolveListExpr(Make<ArgList>(), method->classType);
      auto This = Make<LoadExpr>(Make<VarExpr>(method->formalArgList[0].get()));
      method->stmts->Prepend(Make<StoreStmt>(This, Widen(initializer, method->classType)));
      method->stmts->Append(Make<ReturnStatement>(This));
    }
    symbols_.PopScope();

    if (method->returnType->ContainsRawPtr() && !method->IsConstructor()) {
      Error("cannot return a raw pointer");
    }

    // If last statement is not a return statement,
    if (!method->stmts->ContainsReturn()) {
      if (method->returnType != types_->GetVoid()) {
        ScopedFileLocation scopedFile(&fileLocation_, method->stmts->GetFileLocation());
        Error("implicit void return, in method returning %s.",
          method->returnType->ToString().c_str());
      } else {
        method->stmts->Append(Make<ReturnStatement>(nullptr));
      }
    }
  }

  const auto& fields = classType->GetFields();
  for (const auto& field : fields) {
    if (field->type->IsUnsizedArray() && field != fields.back()) {
      Error("unsized arrays are only allowed as the last field of a class");
    }
  }
  symbols_.PopScope();
  return nullptr;
}

void SemanticPass::UnwindStack(Stmts* stmts) {
  Stmts* topScope = currentMethod_ ? currentMethod_->stmts : nullptr;
  auto stack = symbols_.Get();
  for (auto it = stack.rbegin(); it != stack.rend() && *it != topScope; ++it) {
    for (auto var : (*it)->GetVars()) {
      if (var->type->NeedsDestruction()) {
        stmts->Append(Make<DestroyStmt>(Make<VarExpr>(var.get())));
      }
    }
  }
}

Result SemanticPass::Visit(ReturnStatement* stmt) {
  if (auto returnValue = Resolve(stmt->GetExpr())) {
    auto type = returnValue->GetType(types_);
    auto returnType = currentMethod_ ? currentMethod_->returnType : types_->GetVoid();
    if (!type->CanWidenTo(returnType)) {
      Error("cannot return a value of type %s from a function of type %s", type->ToString().c_str(),
      returnType->ToString().c_str());
    }
  }
  auto stmts = Make<Stmts>();
  UnwindStack(stmts);
  stmts->Append(Make<ReturnStatement>(Resolve(stmt->GetExpr())));
  return stmts;
}

int SemanticPass::FindFormalArg(Arg* arg, Method* m, TypeTable* types) {
  for (int i = 0; i < m->formalArgList.size(); ++i) {
    Var* formalArg = m->formalArgList[i].get();
    if (arg->GetID() == formalArg->name &&
        arg->GetExpr()->GetType(types)->CanWidenTo(formalArg->type)) {
      return i;
    }
  }
  return -1;
}

bool SemanticPass::MatchArgs(Expr*               thisExpr,
                             ArgList*            args,
                             Method*             m,
                             TypeTable*          types,
                             std::vector<Expr*>* newArgList) {
  std::vector<Expr*> result(m->formalArgList.size());
  int                offset = m->modifiers & Method::Modifier::Static ? 0 : 1;
  if (args->IsNamed()) {
    for (auto arg : args->GetArgs()) {
      int index = FindFormalArg(arg, m, types);
      if (index == -1) { return false; }
      result[index] = arg->GetExpr();
    }
  } else {
    size_t      numArgs = args->GetArgs().size();
    Arg* const* a = args->GetArgs().data();
    if (numArgs + offset > m->formalArgList.size()) { return false; }
    for (int i = 0; i < numArgs; ++i) {
      Expr* expr = a[i]->GetExpr();
      if (expr && !expr->GetType(types)->CanWidenTo(m->formalArgList[i + offset]->type)) {
        return false;
      }
      result[i + offset] = expr;
    }
  }
  if (!(m->modifiers & Method::Modifier::Static) && thisExpr && !thisExpr->GetType(types)->CanWidenTo(m->formalArgList[0]->type)) {
    return false;
  }
  for (int i = offset; i < result.size(); ++i) {
    if (!result[i]) {
      if (!m->defaultArgs[i]) { return false; }
      copyFileLocation_ = false;
      // Override the file location by resolving the default args (again).
      result[i] = Resolve(m->defaultArgs[i]);
      copyFileLocation_ = true;
    }
  }
  *newArgList = result;
  return true;
}

Method* SemanticPass::FindMethod(Expr*               thisExpr,
                                 ClassType*          classType,
                                 const std::string&  name,
                                 ArgList*            args,
                                 std::vector<Expr*>* newArgList) {
  for (const auto& it : classType->GetMethods()) {
    Method* m = it.get();
    if (m->name == name) {
      std::vector<Expr*> result;
      if (MatchArgs(thisExpr, args, m, types_, &result)) {
        *newArgList = result;
        return m;
      }
    }
  }
  if (classType->GetParent()) {
    return FindMethod(thisExpr, classType->GetParent(), name, args, newArgList);
  } else {
    return nullptr;
  }
}

static bool MatchAllButFirst(const VarVector& v1, const VarVector& v2) {
  if (v1.size() != v2.size()) { return false; }

  for (size_t i = 1; i < v1.size(); ++i) {
    if (v1[i]->type != v2[i]->type) { return false; }
  }
  return true;
}

Method* SemanticPass::FindOverriddenMethod(ClassType* classType, Method* method) {
  if (!classType) { return nullptr; }

  for (const auto& m : classType->GetMethods()) {
    if (m->name == method->name && MatchAllButFirst(m->formalArgList, method->formalArgList)) {
      return m.get();
    }
  }
  return FindOverriddenMethod(classType->GetParent(), method);
}

Result SemanticPass::Default(ASTNode* node) {
  return Error("Internal compiler error:  attempt to check semantics on a resolved expression.");
}

Result SemanticPass::Error(const char* fmt, ...) {
  const FileLocation& location = fileLocation_;
  std::string         filename = fileLocation_.filename
                                     ? std::filesystem::path(*fileLocation_.filename).filename().string()
                                     : "";
  va_list             argp;
  va_start(argp, fmt);
  fprintf(stderr, "%s:%d:  ", filename.c_str(), fileLocation_.lineNum);
  vfprintf(stderr, fmt, argp);
  fprintf(stderr, "\n");
  numErrors_++;
  return nullptr;
}

Expr* SemanticPass::AutoDereference(Expr* expr) {
  auto type = expr->GetType(types_);
  assert(type->IsRawPtr());
  type = static_cast<RawPtrType*>(type)->GetBaseType();
  type = type->GetUnqualifiedType();
  if (type->IsStrongPtr() || type->IsWeakPtr()) {
    expr = MakeLoad(expr);
    return Make<SmartToRawPtr>(expr);
  }
  return expr;
}

};  // namespace Toucan
