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
  Type* exprType = expr->GetType(types_);
  if (exprType->IsRawPtr()) { exprType = static_cast<RawPtrType*>(exprType)->GetBaseType(); }
  if (exprType->IsStrongPtr() || exprType->IsWeakPtr()) {
    exprType = static_cast<PtrType*>(exprType)->GetBaseType();
    if (expr->GetType(types_)->IsRawPtr()) { expr = Make<LoadExpr>(expr); }
    expr = Make<SmartToRawPtr>(expr);
  }
  exprType = exprType->GetUnqualifiedType();
  Type* indexType = index->GetType(types_);
  if (!indexType->IsInt() && !indexType->IsUInt()) {
    return Error("array index must be integer");
  } else if (exprType->IsArray() || exprType->IsMatrix() || exprType->IsVector()) {
    return Make<ArrayAccess>(expr, index);
  } else {
    return Error("expression is not of indexable type");
  }
}

Result SemanticPass::Visit(CastExpr* node) {
  Expr* expr = Resolve(node->GetExpr());
  if (!expr) { return nullptr; }

  Type* srcType = expr->GetType(types_);
  Type* dstType = node->GetType();
  if (srcType == dstType) {
    return expr;
  } else if (srcType->CanWidenTo(dstType) || srcType->CanNarrowTo(dstType)) {
    return Make<CastExpr>(dstType, expr);
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
    // Append vars to new stmts for any vars in this scope.  Also
    // append destructor calls for any vars that need it.
    symbols_->PopScope();
    for (auto p : scope->vars) {
      newStmts->AppendVar(p.second);
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
  if (type->IsClass() && node->IsConstructor()) {
    ClassType*         classType = static_cast<ClassType*>(type);
    TypeList           types;
    std::vector<Expr*> constructorArgs;
    Method* constructor = FindMethod(nullptr, classType, classType->GetName(), argList, &constructorArgs);
    if (!constructor) {
      return Error("constructor for class \"%s\" with those arguments not found",
                   classType->GetName().c_str());
    }
    auto* tempVar = Make<TempVarExpr>(node->GetType());
    constructorArgs[0] = Make<RawToWeakPtr>(tempVar);
    WidenArgList(constructorArgs, constructor->formalArgList);
    auto* exprList = Make<ExprList>(std::move(constructorArgs));
    Expr* result = Make<MethodCall>(constructor, exprList);
    result = Make<SmartToRawPtr>(result);
    return Make<LoadExpr>(result);
  } else if (type->IsVector() && args.size() == 1) {
    unsigned int length = static_cast<VectorType*>(type)->GetLength();
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

Stmt* SemanticPass::InitializeVar(Expr* varExpr, Type* type, Expr* initExpr) {
  if (initExpr) {
    Type* initExprType = initExpr->GetType(types_);
    if (!initExprType->CanWidenTo(type)) {
      Error("cannot store a value of type \"%s\" to a location of type \"%s\"",
            initExprType->ToString().c_str(), type->ToString().c_str());
      return nullptr;
    }
    initExpr = Widen(initExpr, type);
    return Make<StoreStmt>(varExpr, initExpr);
  } else if (type->IsClass()) {
    return InitializeClass(varExpr, static_cast<ClassType*>(type));
  } else {
    return Make<ZeroInitStmt>(varExpr);
  }
}

Stmts* SemanticPass::InitializeClass(Expr* thisExpr, ClassType* classType) {
  Stmts* stmts = Make<Stmts>();
  if (classType->GetParent()) { stmts->Append(InitializeClass(thisExpr, classType->GetParent())); }
  for (const auto& field : classType->GetFields()) {
    Expr* fieldExpr = Make<FieldAccess>(thisExpr, field.get());
    stmts->Append(InitializeVar(fieldExpr, field->type, Resolve(field->defaultValue)));
  }
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
  Var*  var = symbols_->DefineVar(id, type);
  Expr* varExpr = Make<VarExpr>(var);
  return InitializeVar(varExpr, type, initExpr);
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
  } else {
    return Make<CastExpr>(dstType, node);
  }
}

Expr* SemanticPass::ResolveListExpr(UnresolvedListExpr* node, Type* dstType) {
  if (dstType->IsPtr()) {
    dstType = static_cast<PtrType*>(dstType)->GetBaseType();
    auto* tempVar = Make<TempVarExpr>(dstType, ResolveListExpr(node, dstType));
    return Make<RawToWeakPtr>(tempVar);
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
    if (dstType->IsVector()) {
      elementType = static_cast<VectorType*>(dstType)->GetComponentType();
    } else if (dstType->IsArray()) {
      elementType = static_cast<ArrayType*>(dstType)->GetElementType();
    } else if (dstType->IsMatrix()) {
      elementType = static_cast<MatrixType*>(dstType)->GetColumnType();
    } else {
      assert(!"unexpected type in arglist");
    }
    for (auto arg : argList->GetArgs()) {
      Expr* argExpr = arg->GetExpr();
      if (argExpr->IsUnresolvedListExpr()) {
        argExpr = ResolveListExpr(static_cast<UnresolvedListExpr*>(argExpr), elementType);
      }
      exprs.push_back(argExpr);
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
    expr = Make<RawToWeakPtr>(expr);
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
    return Make<VarExpr>(var);
  } else if (Field* field = symbols_->FindField(id)) {
    Var* thisPtr = symbols_->FindVar("this");
    if (!thisPtr) {
      // TODO:  Implement static field access
      return Error("attempt to access non-static field in static method");
    } else {
      Expr* varExpr = Make<VarExpr>(thisPtr);
      Expr* loadExpr = Make<LoadExpr>(varExpr);
      Expr* derefExpr = Make<SmartToRawPtr>(loadExpr);
      return Make<FieldAccess>(derefExpr, field);
    }
  } else if (const EnumValue* enumValue = symbols_->FindEnumValue(id)) {
    return Make<EnumConstant>(enumValue);
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
  if (type->IsPtr()) {
    type = static_cast<PtrType*>(type)->GetBaseType();
    if (type->IsArray()) {
      if (id == "length") { return Make<LengthExpr>(expr); }
    }
    if (expr->GetType(types_)->IsRawPtr()) { expr = Make<LoadExpr>(expr); }
    expr = Make<SmartToRawPtr>(expr);
  }
  type = type->GetUnqualifiedType();
  if (type->IsArray()) {
    if (id == "length") {
      ArrayType* atype = static_cast<ArrayType*>(type);
      assert(atype->GetNumElements() > 0);
      return Make<IntConstant>(atype->GetNumElements(), 32);  // FIXME: uint?
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
  } else {
    rhs = Widen(rhs, lhsType);
    return Make<StoreStmt>(lhs, rhs);
  }
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
      lhs = Make<CastExpr>(rhsType, lhs);
      assert(lhs->GetType(types_) == rhs->GetType(types_));
    } else if (rhsType->CanWidenTo(lhsType)) {
      rhs = Make<CastExpr>(lhsType, rhs);
      assert(lhs->GetType(types_) == rhs->GetType(types_));
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
  ArgList* arglist = Resolve(node->GetArgList());
  if (!arglist) return nullptr;
  Method*   constructor = nullptr;
  ExprList* args = nullptr;
  int       qualifiers;
  Type*     unqualifiedType = type->GetUnqualifiedType(&qualifiers);
  Expr*     length = node->GetLength() ? Resolve(node->GetLength()) : nullptr;
  if (unqualifiedType->IsClass()) {
    auto* classType = static_cast<ClassType*>(unqualifiedType);
    if (classType->HasUnsizedArray()) {
      if (!length) { return Error("class with unsized array must be allocated with size"); }
    }
    if (!classType->IsFullySpecified()) {
      return Error("cannot allocate partially-specified template class %s",
                   classType->ToString().c_str());
    }
    std::vector<Expr*> exprList;
    constructor = FindMethod(nullptr, classType, classType->GetName(), arglist, &exprList);
    if (constructor) {
      for (int i = 1; i < exprList.size(); ++i) {
        if (!exprList[i]) {
          return Error("formal parameter \"%s\" has no default value",
                       constructor->formalArgList[i]->name.c_str());
        }
      }
      WidenArgList(exprList, constructor->formalArgList);
      args = Make<ExprList>(std::move(exprList));
    } else if (arglist->GetArgs().size() > 0) {
      return Error("matching constructor not found");
    }
  }
  return Make<NewExpr>(type, length, constructor, args);
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

Result SemanticPass::Visit(NewArrayExpr* expr) {
  Expr* sizeExpr = Resolve(expr->GetSizeExpr());
  Type* type = expr->GetElementType();
  if (!type) return nullptr;
  if (type->IsArray()) {
    ArrayType* arrayType = static_cast<ArrayType*>(type);
    if (arrayType->GetNumElements() == 0) { return Error("cannot allocate unsized array"); }
  }
  return Make<NewArrayExpr>(type, sizeExpr);
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
          Stmt* last = Make<ReturnStatement>(nullptr, nullptr);
          method->stmts->Append(last);
        }
      }
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
    destructor->AddFormalArg("this", types_->GetWeakPtrType(classType), nullptr);
    classType->AddMethod(destructor);
  }

  classType->SetVTable(0, destructor);

  if (!destructor->stmts) {
    Stmts* stmts = Make<Stmts>();
    stmts->Append(Make<ReturnStatement>(nullptr, nullptr));
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
