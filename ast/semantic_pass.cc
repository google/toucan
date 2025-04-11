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

#include "symbol.h"
#include "type_replacement_pass.h"

namespace Toucan {

SemanticPass::SemanticPass(NodeVector* nodes, SymbolTable* symbols, TypeTable* types)
    : CopyVisitor(nodes), symbols_(symbols), types_(types), numErrors_(0) {}

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

  if (auto indexableExpr = MakeIndexable(expr)) {
    expr = indexableExpr;
  } else {
    Error("expression is not of indexable type");
  }

  return Make<ArrayAccess>(expr, index);
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

Result SemanticPass::Visit(Stmts* stmts) {
  Stmts* newStmts = Make<Stmts>();
  Scope* scope = stmts->GetScope();
  if (scope) { symbols_->PushScope(scope); }
  for (Stmt* const& it : stmts->GetStmts()) {
    Stmt* stmt = Resolve(it);
    if (stmt) newStmts->Append(stmt);
  }
  if (scope) {
    bool containsReturn = stmts->ContainsReturn();
    // Append vars to new stmts for any vars in this scope.  Also
    // append destructor calls for any vars that need it.
    symbols_->PopScope();
    for (auto p : scope->vars) {
      auto var = p.second;
      newStmts->AppendVar(var);
      if (var->type->NeedsDestruction() && !containsReturn) {
        newStmts->Append(Make<DestroyStmt>(Make<VarExpr>(var.get())));
      }
    }
  }
  return newStmts;
}

Result SemanticPass::Visit(ArgList* node) {
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
  }
  return CopyVisitor::Visit(node);
}

Result SemanticPass::Visit(UnresolvedInitializer* node) {
  Type*              type = node->GetType();
  ArgList*           argList = Resolve(node->GetArgList());
  auto               args = argList->GetArgs();
  std::vector<Expr*> exprs;
  if (type->ContainsRawPtr()) { return Error("cannot allocate a type containing a raw pointer"); }
  if (type->IsClass() && node->IsConstructor()) {
    ClassType*         classType = static_cast<ClassType*>(type);
    TypeList           types;
    std::vector<Expr*> constructorArgs;
    Method* constructor = FindMethod(nullptr, classType, classType->GetName(), argList, &constructorArgs);
    if (!constructor) {
      return Error("constructor for class \"%s\" with those arguments not found",
                   classType->GetName().c_str());
    }
    constructorArgs[0] = Make<TempVarExpr>(node->GetType());
    WidenArgList(constructorArgs, constructor->formalArgList);
    auto* exprList = Make<ExprList>(std::move(constructorArgs));
    Expr* result = Make<MethodCall>(constructor, exprList);
    return Make<LoadExpr>(result);
  } else if (type->IsVector() && args.size() == 1) {
    unsigned int length = static_cast<VectorType*>(type)->GetLength();
    for (int i = 0; i < length; ++i) {
      exprs.push_back(args[0]->GetExpr());
    }
  } else if (type->IsArray() && args.size() == 1) {
    unsigned int length = static_cast<ArrayType*>(type)->GetNumElements();
    for (int i = 0; i < length; ++i) {
      exprs.push_back(args[0]->GetExpr());
    }
  } else if (type->IsMatrix() && args.size() == 1) {
    unsigned int length = static_cast<MatrixType*>(type)->GetNumColumns();
    for (int i = 0; i < length; ++i) {
      exprs.push_back(args[0]->GetExpr());
    }
  } else {
    for (auto arg : args) {
      exprs.push_back(arg->GetExpr());
    }
  }
  auto exprList = Make<ExprList>(std::move(exprs));
  return Make<Initializer>(node->GetType(), exprList);
}

Stmt* SemanticPass::Initialize(Expr* dest, Expr* initExpr) {
  auto type = dest->GetType(types_);
  assert(type->IsRawPtr());
  type = static_cast<RawPtrType*>(type)->GetBaseType();
  if (initExpr) {
    Type* initExprType = initExpr->GetType(types_);
    if (!initExprType->CanWidenTo(type)) {
      Error("cannot store a value of type \"%s\" to a location of type \"%s\"",
            initExprType->ToString().c_str(), type->ToString().c_str());
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
    stmts->Append(Initialize(fieldExpr, Resolve(field->defaultValue)));
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
    auto value = Make<Initializer>(type, exprList);
    auto rhs = Make<LoadExpr>(Make<ArrayAccess>(MakeIndexable(value), Make<LoadExpr>(index)));
    body = Make<StoreStmt>(lhs, rhs);
  }
  stmts->Append(Make<ForStatement>(initStmt, cond, loopStmt, body));
  return stmts;
}

Result SemanticPass::Visit(VarDeclaration* decl) {
  std::string id = decl->GetID();
  Type*       type = decl->GetType();
  if (symbols_->FindVarInScope(id)) {
    return Error("variable \"%s\" already defined in this scope", id.c_str());
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
  Var*  var = symbols_->DefineVar(id, type);
  Expr* varExpr = Make<VarExpr>(var);
  return Initialize(varExpr, initExpr);
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
  return Make<MethodCall>(method, exprList);
}

Expr* SemanticPass::Widen(Expr* node, Type* dstType) {
  Type* srcType = node->GetType(types_);
  if (srcType == dstType) {
    return node;
  } else if (node->IsUnresolvedListExpr()) {
    return ResolveListExpr(static_cast<UnresolvedListExpr*>(node), dstType);
  } else if ((srcType->IsStrongPtr() || srcType->IsWeakPtr()) && dstType->IsRawPtr()) {
    return Make<SmartToRawPtr>(node);
  } else if (dstType->IsRawPtr() && static_cast<RawPtrType*>(dstType)->GetBaseType()->IsArray()) {
    return MakeIndexable(node);
  } else {
    return Make<CastExpr>(dstType, node);
  }
}

Expr* SemanticPass::MakeIndexable(Expr* expr) {
  Type* type = expr->GetType(types_);
  if (type->IsStrongPtr() || type->IsWeakPtr()) {
    return Make<SmartToRawPtr>(expr);
  } else if (!type->IsRawPtr()) {
    return MakeIndexable(Make<TempVarExpr>(type, expr));
  }
  type = static_cast<RawPtrType*>(type)->GetBaseType();
  if (type->IsUnsizedArray()) {
    return expr;
  } else if (type->IsStrongPtr() || type->IsWeakPtr()) {
    return Make<SmartToRawPtr>(Make<LoadExpr>(expr));
  }
  int length;
  Type* elementType;
  MemoryLayout memoryLayout = MemoryLayout::Default;
  if (type->IsMatrix()) {
    auto matrixType = static_cast<MatrixType*>(type);
    length = matrixType->GetNumColumns();
    elementType = matrixType->GetColumnType();
  } else if (type->IsVector()) {
    auto vectorType = static_cast<VectorType*>(type);
    length = vectorType->GetLength();
    elementType = vectorType->GetComponentType();
  } else if (type->IsArray()) {
    auto arrayType = static_cast<ArrayType*>(type);
    length = arrayType->GetNumElements();
    elementType = arrayType->GetElementType();
    memoryLayout = arrayType->GetMemoryLayout();
  } else {
    return nullptr;
  }
  return Make<ToRawArray>(expr, Make<IntConstant>(length, 32), elementType, memoryLayout);
}

Expr* SemanticPass::ResolveListExpr(UnresolvedListExpr* node, Type* dstType) {
  if (dstType->IsRawPtr()) {
    auto baseType = static_cast<RawPtrType*>(dstType)->GetBaseType();
    if (baseType->IsUnsizedArray()) {
      // Resolve as &[N] of list length, then convert to &[]
      auto arrayType = static_cast<ArrayType*>(baseType);
      auto length = node->GetArgList()->GetArgs().size();
      dstType = types_->GetArrayType(arrayType->GetElementType(), length, arrayType->GetMemoryLayout());
      dstType = types_->GetRawPtrType(dstType);
      return MakeIndexable(ResolveListExpr(node, dstType));
    }
    return Make<TempVarExpr>(baseType, ResolveListExpr(node, baseType));
  }
  Type*              type = dstType;
  auto               argList = node->GetArgList();
  std::vector<Expr*> exprs;
  if (dstType->IsClass()) {
    auto  classType = static_cast<ClassType*>(dstType);
    auto& fields = classType->GetFields();
    if (argList->IsNamed()) {
      exprs.resize(classType->GetTotalFields(), nullptr);
      for (auto arg : argList->GetArgs()) {
        Field* field = classType->FindField(arg->GetID());
        exprs[field->index] = Widen(arg->GetExpr(), field->type);
      }
      for (ClassType* c = classType; c != nullptr; c = c->GetParent()) {
        for (auto& field : c->GetFields()) {
          if (exprs[field->index] == nullptr) {
            exprs[field->index] = Resolve(field->defaultValue);
          }
        }
      }
    } else {
      int i = 0;
      for (auto arg : argList->GetArgs()) {
        exprs.push_back(Widen(arg->GetExpr(), fields[i++]->type));
      }
    }
  } else {
    if (argList->IsNamed()) {
      Error("named list expression are unsupported for %s", dstType->ToString().c_str());
      return nullptr;
    }
    Type* elementType;
    int length;
    if (dstType->IsVector()) {
      auto vectorType = static_cast<VectorType*>(dstType);
      elementType = vectorType->GetComponentType();
      length = vectorType->GetLength();
    } else if (dstType->IsArray()) {
      auto arrayType = static_cast<ArrayType*>(dstType);
      elementType = arrayType->GetElementType();
      length = arrayType->GetNumElements();
    } else if (dstType->IsMatrix()) {
      auto matrixType = static_cast<MatrixType*>(dstType);
      elementType = matrixType->GetColumnType();
      length = matrixType->GetNumColumns();
    } else {
      assert(!"unexpected type in arglist");
    }
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
  return Make<Initializer>(type, exprList);
}

Result SemanticPass::Visit(UnresolvedMethodCall* node) {
  std::string id = node->GetID();
  Expr*       expr = Resolve(node->GetExpr());
  if (!expr) return nullptr;
  ArgList* arglist = Resolve(node->GetArgList());
  if (!arglist) return nullptr;
  Type* type = expr->GetType(types_);
  Type* thisPtrType;
  if (type->IsRawPtr()) { type = static_cast<RawPtrType*>(type)->GetBaseType(); }
  if (type->IsPtr()) {
    thisPtrType = type;
    type = static_cast<PtrType*>(type)->GetBaseType();
    if (expr->GetType(types_)->IsRawPtr()) { expr = Make<LoadExpr>(expr); }
  } else {
    if (!expr->GetType(types_)->IsRawPtr()) {
      expr = Make<TempVarExpr>(expr->GetType(types_), expr);
    }
    thisPtrType = expr->GetType(types_);
  }
  if (!type) { return Error("calling method on void pointer?"); }
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

Result SemanticPass::Visit(LoadExpr* node) {
  Expr* expr = Resolve(node->GetExpr());
  if (!expr) return nullptr;
  if (expr->IsUnresolvedSwizzleExpr()) {
    auto swizzle = static_cast<UnresolvedSwizzleExpr*>(expr);
    // Bypass the swizzle, load its base and extract our element.
    Expr* loadedBase = Make<LoadExpr>(swizzle->GetExpr());
    return Make<ExtractElementExpr>(loadedBase, swizzle->GetIndex());
  }
  if (!expr->GetType(types_)->IsRawPtr()) {
    // Method calls and length expressions can get a spurious load when converted from
    // "assignable".
    return expr;
  }
  return Make<LoadExpr>(expr);
}

Result SemanticPass::Visit(UnresolvedIdentifier* node) {
  std::string id = node->GetID();
  if (Var* var = symbols_->FindVar(id)) {
    if (var->type->IsRawPtr()) {
      return Make<LoadExpr>(Make<VarExpr>(var));
    } else {
      return Make<VarExpr>(var);
    }
  } else if (Field* field = symbols_->FindField(id)) {
    Var* thisPtr = symbols_->FindVar("this");
    if (!thisPtr) {
      // TODO:  Implement static field access
      return Error("attempt to access non-static field in static method");
    } else {
      Expr* base = Make<LoadExpr>(Make<VarExpr>(thisPtr));
      return Make<FieldAccess>(base, field);
    }
  } else {
    return Error("unknown symbol \"%s\"", id.c_str());
  }
}

Result SemanticPass::Visit(UnresolvedDot* node) {
  Expr* expr = Resolve(node->GetExpr());
  if (!expr) return nullptr;
  Type* type = expr->GetType(types_);
  if (type->IsRawPtr()) { type = static_cast<RawPtrType*>(type)->GetBaseType(); }
  std::string id = node->GetID();
  if (type->IsStrongPtr() || type->IsWeakPtr()) {
    type = static_cast<PtrType*>(type)->GetBaseType();
    if (expr->GetType(types_)->IsRawPtr()) { expr = Make<LoadExpr>(expr); }
    expr = Make<SmartToRawPtr>(expr);
  }
  type = type->GetUnqualifiedType();
  if (type->IsArray()) {
    if (id == "length") {
      ArrayType* atype = static_cast<ArrayType*>(type);
      if (atype->GetNumElements() > 0) {
        return Make<IntConstant>(atype->GetNumElements(), 32);  // FIXME: uint?
      } else {
        return Make<LengthExpr>(expr);
      }
    } else {
      return Error("unknown array property \"%s\"", id.c_str());
    }
  } else if (type->IsClass()) {
    ClassType* classType;
    classType = static_cast<ClassType*>(type);
    Field* field = classType->FindField(id);
    if (field) {
      return Make<FieldAccess>(expr, field);
    } else {
      return Error("field \"%s\" not found on class \"%s\"", id.c_str(),
                   classType->ToString().c_str());
    }
  } else if (type->IsVector()) {
    int index = static_cast<VectorType*>(type)->GetSwizzle(id);
    if (index >= 0 && index <= 3) {
      return Make<UnresolvedSwizzleExpr>(expr, index);
    } else {
      return Error("invalid swizzle '%s'", id.c_str());
    }
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
      return Make<EnumConstant>(enumValue);
    } else {
      return Error("value \"%s\" not found on enum \"%s\"", id.c_str(),
                   enumType->ToString().c_str());
    }
  } else {
    return Error("expression is not of enum type");
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
  Expr* lhs = Resolve(node->GetLHS());
  if (!lhs) return nullptr;
  Expr* rhs = Resolve(node->GetRHS());
  if (!rhs) return nullptr;
  if (lhs->IsUnresolvedSwizzleExpr()) {
    auto swizzle = static_cast<UnresolvedSwizzleExpr*>(lhs);
    // Bypass the swizzle, load its arg and insert our new value.
    lhs = swizzle->GetExpr();
    Expr* loadedBase = Make<LoadExpr>(lhs);
    rhs = Make<InsertElementExpr>(loadedBase, rhs, swizzle->GetIndex());
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
    return Error("cannot store a value of type \"%s\" to a location of type \"%s\"",
                 rhsType->ToString().c_str(), lhsType->ToString().c_str());
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
    if (lhsType->IsEnum() || lhsType->IsBool()) {
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

Result SemanticPass::Visit(UnresolvedClassDefinition* defn) {
  Scope*     scope = defn->GetScope();
  ClassType* classType = scope->classType;
  // Non-native template classes don't need semantic analysis, since
  // their code won't be directly generated.
  if (!classType->IsNative() && classType->IsClassTemplate()) {
    return nullptr;
  }

  symbols_->PushScope(scope);
  Method* destructor = nullptr;
  if (auto parent = classType->GetParent()) {
    classType->SetVTable(parent->GetVTable());
  }
  for (const auto& mit : classType->GetMethods()) {
    Method* method = mit.get();
    if (method->name[0] == '~') {
      destructor = method;
    } else {
      Method* match = FindOverriddenMethod(classType->GetParent(), method);
      if (match) {
        if (method->modifiers & Method::Modifier::Virtual) {
          if (!(match->modifiers & Method::Modifier::Virtual)) {
            return Error("attempt to override a non-virtual method");
          }
        } else if (match->modifiers & Method::Modifier::Virtual) {
          return Error("override of virtual method must be virtual");
        }
      }
      if (method->modifiers & Method::Modifier::Virtual) {
        if (match) {
          classType->SetVTable(match->index, method);
        } else {
          classType->AppendToVTable(method);
        }
      }
    }

    if (method->stmts) {
      method->stmts = Resolve(method->stmts);
      // If last statement is not a return statement,
      if (!method->stmts->ContainsReturn()) {
        if (method->returnType != types_->GetVoid()) {
          return Error("implicit void return, in method returning non-void.");
        } else {
          UnwindStack(method->stmts->GetScope(), method->stmts);
          method->stmts->Append(Make<ReturnStatement>(nullptr));
        }
      }
    }
    if (method->returnType->ContainsRawPtr() && !method->IsConstructor()) {
      return Error("cannot return a raw pointer");
    }
    for (int i = 0; i < method->defaultArgs.size(); ++i) {
      method->defaultArgs[i] = Resolve(method->defaultArgs[i]);
      if (method->formalArgList[i]->type->IsAuto()) {
        method->formalArgList[i]->type = method->defaultArgs[i]->GetType(types_);
      }
    }
  }

  if (!destructor) {
    std::string name(std::string("~") + classType->GetName());
    destructor = new Method(Method::Modifier::Virtual, types_->GetVoid(), name, classType);
    destructor->AddFormalArg("this", types_->GetRawPtrType(classType), nullptr);
    classType->AddMethod(destructor);
  }

  classType->SetVTable(0, destructor);

  if (!destructor->stmts) {
    Stmts* stmts = Make<Stmts>();
    stmts->Append(Make<ReturnStatement>(nullptr));
    destructor->stmts = stmts;
  }

  const auto& fields = classType->GetFields();
  for (const auto& field : fields) {
    if (field->type->IsAuto()) {
      assert(field->defaultValue);
      field->type = field->defaultValue->GetType(types_);
    } else if (field->type->IsUnsizedArray()) {
      if (field != fields.back()) {
        return Error("Unsized arrays are only allwed as the last field of a class");
      }
    }
  }
  symbols_->PopScope();
  // FIXME:  generate class destructor here?
  return nullptr;
}

void SemanticPass::UnwindStack(Scope* scope, Stmts* stmts) {
  for(; scope && !scope->method && !scope->classType; scope = scope->parent) {
    for (auto p : scope->vars) {
      auto var = p.second;
      if (var->type->NeedsDestruction()) {
        stmts->Append(Make<DestroyStmt>(Make<VarExpr>(var.get())));
      }
    }
  }
}

Result SemanticPass::Visit(ReturnStatement* stmt) {
  if (auto returnValue = Resolve(stmt->GetExpr())) {
    auto type = returnValue->GetType(types_);
    auto scope = symbols_->PeekScope();
    while (scope && !scope->method) { scope = scope->parent; }
    auto returnType = scope ? scope->method->returnType : types_->GetVoid();
    if (!type->CanWidenTo(returnType)) {
      return Error("cannot return a value of type %s from a function of type %s", type->ToString().c_str(),
        returnType->ToString().c_str());
    }
  }
  auto stmts = Make<Stmts>();
  UnwindStack(symbols_->PeekScope(), stmts);
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
    if (!(m->modifiers & Method::Modifier::Static) && thisExpr && !thisExpr->GetType(types)->CanWidenTo(m->formalArgList[0]->type)) {
      return false;
    }
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

};  // namespace Toucan
