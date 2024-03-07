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

#include "constant_folder.h"
#include "symbol.h"
#include "type_replacement_pass.h"

namespace Toucan {

SemanticPass::SemanticPass(NodeVector* nodes, SymbolTable* symbols, TypeTable* types)
    : NodeVisitor(nodes), symbols_(symbols), types_(types), numErrors_(0) {}

Result SemanticPass::Visit(SmartToRawPtr* node) {
  Expr* expr = Resolve(node->GetExpr());
  if (!expr) return nullptr;
  Type* exprType = expr->GetType(types_);
  if (!exprType->IsStrongPtr() && !exprType->IsWeakPtr()) {
    return Error(node, "attempt to dereference a non-pointer");
  }
  return Make<SmartToRawPtr>(node, expr);
}

Result SemanticPass::Visit(AddressOf* node) {
  Expr* expr = Resolve(node->GetExpr());
  if (!expr) return nullptr;
  Type* exprType = expr->GetType(types_);
  return Make<AddressOf>(node, expr);
}

Result SemanticPass::Visit(ArrayAccess* node) {
  if (!node->GetIndex()) { return Error(node, "variable-sized array type used as expression"); }
  Expr* expr = Resolve(node->GetExpr());
  if (!expr) return nullptr;
  Expr* index = Resolve(node->GetIndex());
  if (!index) return nullptr;
  Type* exprType = expr->GetType(types_);
  if (exprType->IsRawPtr()) { exprType = static_cast<RawPtrType*>(exprType)->GetBaseType(); }
  if (exprType->IsStrongPtr() || exprType->IsWeakPtr()) {
    exprType = static_cast<PtrType*>(exprType)->GetBaseType();
    if (expr->GetType(types_)->IsRawPtr()) { expr = Make<LoadExpr>(node, expr); }
    expr = Make<SmartToRawPtr>(node, expr);
  }
  exprType = exprType->GetUnqualifiedType();
  Type* indexType = index->GetType(types_);
  if (!indexType->IsInt() && !indexType->IsUInt()) {
    return Error(node, "array index must be integer");
  } else if (exprType->IsArray() || exprType->IsMatrix()) {
    return Make<ArrayAccess>(node, expr, index);
  } else if (exprType->IsVector()) {
    ConstantFolder cf;
    return Make<UnresolvedSwizzleExpr>(node, expr, cf.Resolve(index));
  } else {
    return Error(node, "expression is not of indexable type");
  }
}

Result SemanticPass::Visit(CastExpr* node) {
  Expr* expr = Resolve(node->GetExpr());
  if (expr && expr->GetType(types_) == node->GetType()) {
    return expr;
  } else {
    return Make<CastExpr>(node, node->GetType(), expr);
  }
}

Result SemanticPass::Visit(Data* node) { return node; }

Result SemanticPass::Visit(Stmts* stmts) {
  Stmts* newStmts = Make<Stmts>(stmts);
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
        return Error(node, "if one argument is named, all arguments must be named");
      }
    } else {
      if (!arg->GetID().empty()) {
        return Error(node, "if one argument is unnamed, all arguments must be unnamed");
      }
    }
  }
  return NodeVisitor::Visit(node);
}

Result SemanticPass::Visit(ConstructorNode* node) {
  Type*    type = node->GetType();
  ArgList* argList = Resolve(node->GetArgList());
  auto     args = argList->GetArgs();
  Method*  constructor;
  if (type->IsVector()) {
    auto vectorType = static_cast<VectorType*>(type);
    if (args.size() > vectorType->GetLength()) {
      return Error(node, "incorrect number of arguments to vector constructor");
    } else if (args.size() == 1) {
      auto* newArgs = Make<ArgList>(node);
      for (int i = 0; i < vectorType->GetLength(); ++i) {
        newArgs->Append(args[0]);
      }
      argList = newArgs;
    }
  } else if (type->IsMatrix()) {
    auto  matrixType = static_cast<MatrixType*>(type);
    Type* columnType = matrixType->GetColumnType();
    if (args.size() != matrixType->GetNumColumns()) {
      return Error(node, "incorrect number of arguments to matrix constructor");
    }
    for (auto arg : args) {
      Type* argType = arg->GetExpr()->GetType(types_);
      if (argType != columnType) {
        return Error(node, "expected type %s in matrix constructor, got %s",
                     columnType->ToString().c_str(), argType->ToString().c_str());
      }
    }
  } else if (type->IsClass()) {
    ClassType*         classType = static_cast<ClassType*>(type);
    TypeList           types;
    std::vector<Expr*> exprList;
    constructor = classType->FindMethod(classType->GetName(), argList, types_, &exprList);
    if (!constructor) {
      return Error(node, "constructor for class \"%s\" with those arguments not found",
                   classType->GetName().c_str());
    }
    WidenArgList(node, exprList, constructor->formalArgList);
  }
  return Make<ConstructorNode>(node, node->GetType(), argList, constructor);
}

Result SemanticPass::Visit(VarDeclaration* decl) {
  std::string id = decl->GetID();
  Type*       type = decl->GetType();
  if (symbols_->FindVarInScope(id)) {
    return Error(decl, "variable \"%s\" already defined in this scope", id.c_str());
  }
  if (!type) return nullptr;
  Expr* initExpr = nullptr;
  if (decl->GetInitExpr()) {
    initExpr = Resolve(decl->GetInitExpr());
    if (!initExpr) { return nullptr; }
  }
  if (type->IsAuto()) {
    if (!initExpr) { return Error(decl, "auto with no initializer expression"); }
    type = initExpr->GetType(types_);
  }
  if (type->IsVoid() || (type->IsArray() && static_cast<ArrayType*>(type)->GetNumElements() == 0)) {
    std::string errorMsg = std::string("cannot create storage of type ") + type->ToString();
    return Error(decl, errorMsg.c_str());
  }
  Var* var = symbols_->DefineVar(id, type);
  if (initExpr) {
    Type* initExprType = initExpr->GetType(types_);
    if (!initExprType->CanWidenTo(type)) {
      return Error(decl, "cannot store a value of type \"%s\" to a location of type \"%s\"",
                   initExprType->ToString().c_str(), type->ToString().c_str());
    }
    Expr* varExpr = Make<VarExpr>(decl, var);
    return Make<StoreStmt>(decl, varExpr, initExpr);
  }
  return nullptr;
}

Result SemanticPass::ResolveMethodCall(ASTNode*    node,
                                       Expr*       expr,
                                       ClassType*  classType,
                                       std::string id,
                                       ArgList*    arglist) {
  std::vector<Expr*> newArgList;
  Method*            method = classType->FindMethod(id, arglist, types_, &newArgList);
  if (!method) {
    std::string msg = "class " + classType->ToString() + " has no method " + id;
    msg += "(";
    for (auto arg : arglist->GetArgs()) {
      if (arg->GetID() != "") { msg += arg->GetID() + " = "; }
      msg += arg->GetExpr()->GetType(types_)->ToString();
      if (arg != arglist->GetArgs().back()) { msg += ", "; }
    }
    msg += ")";
    Error(node, msg.c_str());

    for (const auto& method : classType->GetMethods()) {
      if (method->name == id) { Error(node, method->ToString().c_str()); }
    }
    return nullptr;
  }
  if (!(method->modifiers & Method::STATIC)) {
    if (!expr) {
      return Error(node, "attempt to call non-static method \"%s\" on class \"%s\"", id.c_str(),
                   classType->ToString().c_str());
    } else {
      newArgList[0] = expr;
    }
  }
  for (int i = 0; i < newArgList.size(); ++i) {
    if (!newArgList[i]) {
      return Error(node, "formal parameter \"%s\" has no default value",
                   method->formalArgList[i]->name.c_str());
    }
  }
  WidenArgList(node, newArgList, method->formalArgList);
  auto* exprList = Make<ExprList>(node, std::move(newArgList));
  return Make<MethodCall>(node, method, exprList);
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
    if (expr->GetType(types_)->IsRawPtr()) { expr = Make<LoadExpr>(node, expr); }
  } else {
    expr = Make<AddressOf>(node, expr);
    thisPtrType = expr->GetType(types_);
  }
  if (!type) { return Error(node, "calling method on void pointer?"); }
  type = type->GetUnqualifiedType();
  if (!type->IsClass()) { return Error(node, "expression does not evaluate to class type"); }
  ClassType* classType = static_cast<ClassType*>(type);
  return ResolveMethodCall(node, expr, classType, id, arglist);
}

Result SemanticPass::Visit(UnresolvedStaticMethodCall* node) {
  std::string id = node->GetID();
  ArgList*    arglist = Resolve(node->GetArgList());
  if (!arglist) return nullptr;
  ClassType* classType = node->classType();
  return ResolveMethodCall(node, nullptr, classType, id, arglist);
}

Result SemanticPass::Visit(LoadExpr* node) {
  Expr* expr = Resolve(node->GetExpr());
  if (!expr) return nullptr;
  if (expr->IsUnresolvedSwizzleExpr()) {
    auto swizzle = static_cast<UnresolvedSwizzleExpr*>(expr);
    // Bypass the swizzle, load its base and extract our element.
    Expr* loadedBase = Make<LoadExpr>(node, swizzle->GetExpr());
    return Make<ExtractElementExpr>(node, loadedBase, swizzle->GetIndex());
  }
  if (!expr->GetType(types_)->IsRawPtr()) {
    // Method calls and length expressions can get a spurious load when converted from
    // "assignable".
    return expr;
  }
  return Make<LoadExpr>(node, expr);
}

Result SemanticPass::Visit(UnresolvedIdentifier* node) {
  std::string id = node->GetID();
  if (Var* var = symbols_->FindVar(id)) {
    return Make<VarExpr>(node, var);
  } else if (Field* field = symbols_->FindField(id)) {
    Var* thisPtr = symbols_->FindVar("this");
    if (!thisPtr) {
      // TODO:  Implement static field access
      return Error(node, "attempt to access non-static field in static method");
    } else {
      Expr* varExpr = Make<VarExpr>(node, thisPtr);
      Expr* loadExpr = Make<LoadExpr>(node, varExpr);
      Expr* derefExpr = Make<SmartToRawPtr>(node, loadExpr);
      return Make<FieldAccess>(node, derefExpr, field);
    }
  } else if (const EnumValue* enumValue = symbols_->FindEnumValue(id)) {
    return Make<EnumConstant>(node, enumValue);
  } else {
    return Error(node, "unknown symbol \"%s\"", id.c_str());
  }
}

static int GetSwizzle(Type* type, const std::string& str) {
  if (!type->IsVector()) { return -1; }
  VectorType* vtype = static_cast<VectorType*>(type);
  if (str[0] == '\0') return -1;
  if (str[1] != '\0') return -1;
  char c = str[0];
  if (c == 'x' || c == 'r') return 0;
  if (c == 'y' || c == 'g') return 1;
  if ((c == 'z' || c == 'b') && vtype->GetLength() >= 3) return 2;
  if ((c == 'w' || c == 'a') && vtype->GetLength() >= 4) return 3;
  return -1;
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
      if (id == "length") { return Make<LengthExpr>(node, expr); }
    }
    if (expr->GetType(types_)->IsRawPtr()) { expr = Make<LoadExpr>(node, expr); }
    expr = Make<SmartToRawPtr>(node, expr);
  }
  type = type->GetUnqualifiedType();
  if (type->IsArray()) {
    if (id == "length") {
      ArrayType* atype = static_cast<ArrayType*>(type);
      assert(atype->GetNumElements() > 0);
      return Make<IntConstant>(node, atype->GetNumElements(), 32);  // FIXME: uint?
    } else {
      return Error(node, "unknown array property \"%s\"", id.c_str());
    }
  } else if (type->IsClass()) {
    ClassType* classType;
    classType = static_cast<ClassType*>(type);
    Field* field = classType->FindField(id);
    if (field) {
      return Make<FieldAccess>(node, expr, field);
    } else {
      return Error(node, "field \"%s\" not found on class \"%s\"", id.c_str(),
                   classType->ToString().c_str());
    }
  } else if (type->IsVector()) {
    int index = GetSwizzle(type, id);
    if (index >= 0 && index <= 3) {
      return Make<UnresolvedSwizzleExpr>(node, expr, index);
    } else {
      return Error(node, "invalid swizzle '%s'", id.c_str());
    }
  } else {
    return Error(node, "Expression is not of class, reference or vector type");
  }
}

Result SemanticPass::Visit(IncDecExpr* node) {
  Expr* expr = Resolve(node->GetExpr());
  if (!expr) return nullptr;
  return Make<IncDecExpr>(node, node->GetOp(), expr, node->returnOrigValue());
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
    Expr* loadedBase = Make<LoadExpr>(node, lhs);
    rhs = Make<InsertElementExpr>(node, loadedBase, rhs, swizzle->GetIndex());
  }
  Type* lhsType = lhs->GetType(types_);
  if (!lhsType->IsRawPtr()) { return Error(node, "expression is not an assignable value"); }
  lhsType = static_cast<RawPtrType*>(lhsType)->GetBaseType();
  Type* rhsType = rhs->GetType(types_);
  int   qualifiers;
  lhsType = lhsType->GetUnqualifiedType(&qualifiers);
  if (qualifiers & Type::Qualifier::ReadOnly) {
    return Error(node, "expression is not an assignable value");
  } else if (!rhsType->CanWidenTo(lhsType)) {
    return Error(node, "cannot store a value of type \"%s\" to a location of type \"%s\"",
                 rhsType->ToString().c_str(), lhsType->ToString().c_str());
  } else {
    return Make<StoreStmt>(node, lhs, rhs);
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
    return Error(node, "type mismatch on binary operator");
  }
  if (node->GetOp() == BinOpNode::LOGICAL_AND || node->GetOp() == BinOpNode::LOGICAL_AND) {
    if (!lhsType->IsBool() || !rhsType->IsBool()) {
      return Error(node, "non-boolean argument to logical operator");
    }
  }
  if (node->IsRelOp()) {
    if (lhsType->IsEnum() || lhsType->IsBool()) {
      if (!isEqualityOp) { return Error(node, "invalid type for binary operator"); }
    } else if (!(lhsType->IsInt() || lhsType->IsUInt() || lhsType->IsFloatingPoint())) {
      return Error(node, "invalid type for relational operator");
    }
  }
  if (lhsType != rhsType) {
    if (lhsType->CanWidenTo(rhsType)) {
      lhs = Make<CastExpr>(node, rhsType, lhs);
      assert(lhs->GetType(types_) == rhs->GetType(types_));
    } else if (rhsType->CanWidenTo(lhsType)) {
      rhs = Make<CastExpr>(node, lhsType, rhs);
      assert(lhs->GetType(types_) == rhs->GetType(types_));
    }
  }
  return Make<BinOpNode>(node, node->GetOp(), lhs, rhs);
}

void SemanticPass::WidenArgList(ASTNode*            node,
                                std::vector<Expr*>& argList,
                                const VarVector&    formalArgList) {
  assert(argList.size() == formalArgList.size());
  for (int i = 0; i < argList.size(); ++i) {
    if (!argList[i]) continue;
    Type* argType = argList[i]->GetType(types_);
    Type* expectedArgType = formalArgList[i]->type;
    if (argType != expectedArgType) {
      assert(argType->CanWidenTo(expectedArgType));
      argList[i] = Make<CastExpr>(node, expectedArgType, argList[i]);
    }
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
      if (!length) { return Error(node, "class with unsized array must be allocated with size"); }
    }
    if (!classType->IsFullySpecified()) {
      return Error(node, "cannot allocate partially-specified template class %s",
                   classType->ToString().c_str());
    }
    std::vector<Expr*> exprList;
    constructor = classType->FindMethod(classType->GetName(), arglist, types_, &exprList);
    if (constructor) {
      for (int i = 1; i < exprList.size(); ++i) {
        if (!exprList[i]) {
          return Error(node, "formal parameter \"%s\" has no default value",
                       constructor->formalArgList[i]->name.c_str());
        }
      }
      WidenArgList(node, exprList, constructor->formalArgList);
      args = Make<ExprList>(node, exprList);
    } else if (classType->IsNative()) {
      return Error(node->GetArgList(), "matching constructor not found");
    }
  }
  return Make<NewExpr>(node, type, length, constructor, args);
}

Result SemanticPass::Visit(IfStatement* s) {
  Expr* expr = Resolve(s->GetExpr());
  if (expr && expr->GetType(types_) != types_->GetBool()) {
    return Error(s, "condition must be boolean");
  } else {
    Stmt* stmt = Resolve(s->GetStmt());
    Stmt* optElse = Resolve(s->GetOptElse());
    return Make<IfStatement>(s, expr, stmt, optElse);
  }
}

Result SemanticPass::Visit(WhileStatement* s) {
  Expr* cond = Resolve(s->GetCond());
  Stmt* body = Resolve(s->GetBody());
  if (cond && cond->GetType(types_) != types_->GetBool()) {
    return Error(s, "condition must be boolean");
  } else {
    return Make<WhileStatement>(s, cond, body);
  }
}

Result SemanticPass::Visit(DoStatement* s) {
  Stmt* body = Resolve(s->GetBody());
  Expr* cond = Resolve(s->GetCond());
  if (cond && cond->GetType(types_) != types_->GetBool()) {
    return Error(s, "condition must be boolean");
  } else {
    return Make<DoStatement>(s, body, cond);
  }
}

Result SemanticPass::Visit(ForStatement* node) {
  Stmt* initStmt = Resolve(node->GetInitStmt());
  Expr* cond = Resolve(node->GetCond());
  Stmt* loopStmt = Resolve(node->GetLoopStmt());
  Stmt* body = Resolve(node->GetBody());
  return Make<ForStatement>(node, initStmt, cond, loopStmt, body);
}

Result SemanticPass::Visit(NewArrayExpr* expr) {
  Expr* sizeExpr = Resolve(expr->GetSizeExpr());
  Type* type = expr->GetElementType();
  if (!type) return nullptr;
  if (type->IsArray()) {
    ArrayType* arrayType = static_cast<ArrayType*>(type);
    if (arrayType->GetNumElements() == 0) { return Error(expr, "cannot allocate unsized array"); }
  }
  return Make<NewArrayExpr>(expr, type, sizeExpr);
}

Result SemanticPass::Visit(UnresolvedClassDefinition* defn) {
  Scope*     scope = defn->GetScope();
  ClassType* classType = scope->classType;
  assert(!classType->IsClassTemplate());
  symbols_->PushScope(scope);
  for (const auto& mit : classType->GetMethods()) {
    Method* method = mit.get();
    if (method->stmts) {
      method->stmts = Resolve(method->stmts);
      // If last statement is not a return statement,
      if (!method->stmts->ContainsReturn()) {
        if (method->returnType != types_->GetVoid()) {
          return Error(defn, "implicit void return, in method returning non-void.");
        } else {
          Stmt* last = Make<ReturnStatement>(defn, nullptr, nullptr);
          method->stmts->Append(last);
        }
      }
    }
  }
  Method* destructor = classType->GetVTable()[0];
  if (!destructor->stmts) {
    Stmts* stmts = Make<Stmts>(defn);
    stmts->Append(Make<ReturnStatement>(defn, nullptr, nullptr));
    destructor->stmts = stmts;
  }

  const auto& fields = classType->GetFields();
  for (const auto& field : fields) {
    if (field->type->IsUnsizedArray()) {
      if (field != fields.back()) {
        return Error(defn, "Unsized arrays are only allwed as the last field of a class");
      }
    }
  }
  symbols_->PopScope();
  // FIXME:  generate class destructor here?
  return nullptr;
}

Result SemanticPass::Default(ASTNode* node) {
  return Error(node,
               "Internal compiler error:  attempt to check semantics on a resolved expression.");
}

Result SemanticPass::Error(ASTNode* node, const char* fmt, ...) {
  const FileLocation& location = node->GetFileLocation();
  std::string         filename =
      location.filename ? std::filesystem::path(*location.filename).filename().string() : "";
  va_list argp;
  va_start(argp, fmt);
  fprintf(stderr, "%s:%d:  ", filename.c_str(), location.lineNum);
  vfprintf(stderr, fmt, argp);
  fprintf(stderr, "\n");
  numErrors_++;
  return nullptr;
}

};  // namespace Toucan
