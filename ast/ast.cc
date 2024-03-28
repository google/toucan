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

#include "ast.h"

namespace Toucan {

FileLocation::FileLocation(std::shared_ptr<std::string> f, int n) : filename(f), lineNum(n) {}

ASTNode::ASTNode() : fileLocation_(nullptr, 0) {}

Expr::Expr() {}

Type* LoadExpr::GetType(TypeTable* types) {
  Type* type = expr_->GetType(types);
  assert(type->IsRawPtr());
  return static_cast<RawPtrType*>(type)->GetBaseType()->GetUnqualifiedType();
}

Type* IncDecExpr::GetType(TypeTable* types) {
  Type* type = expr_->GetType(types);
  assert(type->IsRawPtr());
  return static_cast<RawPtrType*>(type)->GetBaseType()->GetUnqualifiedType();
}

Type* VarExpr::GetType(TypeTable* types) { return types->GetRawPtrType(var_->type); }

Type* AddressOf::GetType(TypeTable* types) {
  Type* type = expr_->GetType(types);
  if (type->IsRawPtr()) {
    return types->GetWeakPtrType(static_cast<RawPtrType*>(type)->GetBaseType());
  } else {
    return types->GetWeakPtrType(type);
  }
}

Type* SmartToRawPtr::GetType(TypeTable* types) {
  Type* type = expr_->GetType(types);
  assert(type->IsStrongPtr() || type->IsWeakPtr());
  return types->GetRawPtrType(static_cast<PtrType*>(type)->GetBaseType());
}

Type* FieldAccess::GetType(TypeTable* types) {
  Type* baseType = expr_->GetType(types);
  if (baseType->IsRawPtr()) { baseType = static_cast<RawPtrType*>(baseType)->GetBaseType(); }
  int qualifiers;
  baseType->GetUnqualifiedType(&qualifiers);
  return types->GetRawPtrType(types->GetQualifiedType(field_->type, qualifiers));
}

Type* ArrayAccess::GetType(TypeTable* types) {
  Type* type = expr_->GetType(types);
  if (type->IsPtr()) { type = static_cast<PtrType*>(type)->GetBaseType(); }
  int qualifiers;
  type = type->GetUnqualifiedType(&qualifiers);
  Type* result;
  if (type->IsArray()) {
    result = static_cast<ArrayType*>(type)->GetElementType();
  } else if (type->IsVector()) {
    result = static_cast<VectorType*>(type)->GetComponentType();
  } else if (type->IsMatrix()) {
    result = static_cast<MatrixType*>(type)->GetColumnType();
  } else {
    assert(false);
  }
  return types->GetRawPtrType(types->GetQualifiedType(result, qualifiers));
}

ASTNode::~ASTNode() {}

Arg::Arg(std::string id, Expr* expr) : id_(id), expr_(expr) {}

ArgList::ArgList() {}

ArgList::ArgList(std::vector<Arg*>&& args) : args_(std::move(args)) {}

bool ArgList::IsNamed() const { return !args_.empty() && !args_.front()->GetID().empty(); }

BinOpNode::BinOpNode(Op op, Expr* lhs, Expr* rhs) : op_(op), lhs_(lhs), rhs_(rhs) {}

bool BinOpNode::IsRelOp() const {
  return op_ == LT || op_ == GT || op_ == LE || op_ == GE || op_ == EQ || op_ == NE;
}

Type* BinOpNode::GetType(TypeTable* types) {
  if (IsRelOp()) {
    return types->GetBool();
  } else {
    Type* lhsType = lhs_->GetType(types);
    Type* rhsType = rhs_->GetType(types);
    if (lhsType == rhsType) { return lhsType; }
    if (lhsType->IsVector()) {
      return lhsType;
    } else if (rhsType->IsVector()) {
      return rhsType;
    } else if (lhsType->IsMatrix()) {
      return lhsType;
    } else if (rhsType->IsMatrix()) {
      return rhsType;
    } else if (lhsType->IsInt() && rhsType->IsUInt()) {
      return rhsType;
    } else if (lhsType->IsUInt() && rhsType->IsInt()) {
      return lhsType;
    } else {
      assert(false);
      return lhsType;
    }
  }
}

UnaryOp::UnaryOp(Op op, Expr* rhs) : op_(op), rhs_(rhs) {}

Type* UnaryOp::GetType(TypeTable* types) { return rhs_->GetType(types); }

ConstructorNode::ConstructorNode(Type* type, ArgList* arglist)
    : type_(type), arglist_(arglist) {}

UnresolvedListExpr::UnresolvedListExpr(ArgList* arglist) : arglist_(arglist) {}

Type* UnresolvedListExpr::GetType(TypeTable* types) {
  VarVector vars;
  for (auto arg : arglist_->GetArgs()) {
    vars.push_back(std::make_shared<Var>(arg->GetID(), arg->GetExpr()->GetType(types)));
  }
  return types->GetList(std::move(vars));
}

VarDeclaration::VarDeclaration(std::string id, Type* type, Expr* initExpr)
    : id_(id), type_(type), initExpr_(initExpr) {}

ArrayAccess::ArrayAccess(Expr* expr, Expr* index) : expr_(expr), index_(index) {}

UnresolvedMethodCall::UnresolvedMethodCall(Expr* expr, std::string id, ArgList* arglist)
    : expr_(expr), id_(id), arglist_(arglist) {}

UnresolvedStaticMethodCall::UnresolvedStaticMethodCall(ClassType*  classType,
                                                       std::string id,
                                                       ArgList*    arglist)
    : classType_(classType), id_(id), arglist_(arglist) {}

MethodCall::MethodCall(Method* method, ExprList* arglist) : method_(method), arglist_(arglist) {}

VarExpr::VarExpr(Var* var) : var_(var) {}

LoadExpr::LoadExpr(Expr* expr) : expr_(expr) {}

StoreStmt::StoreStmt(Expr* lhs, Expr* rhs) : lhs_(lhs), rhs_(rhs) {}

IncDecExpr::IncDecExpr(Op op, Expr* expr, bool returnOrigValue)
    : op_(op), expr_(expr), returnOrigValue_(returnOrigValue) {}

ZeroInitStmt::ZeroInitStmt(Expr* lhs) : lhs_(lhs) {}

UnresolvedDot::UnresolvedDot(Expr* expr, std::string id) : expr_(expr), id_(id) {}

AddressOf::AddressOf(Expr* expr) : expr_(expr) {}

SmartToRawPtr::SmartToRawPtr(Expr* expr) : expr_(expr) {}

FieldAccess::FieldAccess(Expr* expr, Field* field) : expr_(expr), field_(field) {}

UnresolvedIdentifier::UnresolvedIdentifier(std::string id) : id_(id) {}

LengthExpr::LengthExpr(Expr* expr) : expr_(expr) {}

Type* LengthExpr::GetType(TypeTable* types) { return types->GetInt(); }

UnresolvedSwizzleExpr::UnresolvedSwizzleExpr(Expr* expr, int index) : expr_(expr), index_(index) {}

Type* UnresolvedSwizzleExpr::GetType(TypeTable* types) {
  Type* type = expr_->GetType(types);
  if (!type) return nullptr;
  if (!type->IsVector()) return nullptr;
  VectorType* vtype = static_cast<VectorType*>(type);
  return vtype->GetComponentType();
}

ExtractElementExpr::ExtractElementExpr(Expr* expr, int index) : expr_(expr), index_(index) {}

Type* ExtractElementExpr::GetType(TypeTable* types) {
  Type* type = expr_->GetType(types)->GetUnqualifiedType();
  assert(type->IsVector());
  return static_cast<VectorType*>(type)->GetComponentType();
}

InsertElementExpr::InsertElementExpr(Expr* expr, Expr* newElement, int index)
    : expr_(expr), newElement_(newElement), index_(index) {}

Type* InsertElementExpr::GetType(TypeTable* types) { return expr_->GetType(types); }

IntConstant::IntConstant(int32_t value, uint32_t bits) : value_(value), bits_(bits) {}

Type* IntConstant::GetType(TypeTable* types) { return types->GetInteger(bits_, true); }

UIntConstant::UIntConstant(uint32_t value, uint32_t bits) : value_(value), bits_(bits) {}

Type* UIntConstant::GetType(TypeTable* types) { return types->GetInteger(bits_, false); }

FloatConstant::FloatConstant(float value) : value_(value) {}

Type* FloatConstant::GetType(TypeTable* types) { return types->GetFloat(); }

DoubleConstant::DoubleConstant(double value) : value_(value) {}

Type* DoubleConstant::GetType(TypeTable* types) { return types->GetDouble(); }

BoolConstant::BoolConstant(bool value) : value_(value) {}

Type* BoolConstant::GetType(TypeTable* types) { return types->GetBool(); }

Data::Data(Type* type, std::unique_ptr<uint8_t[]> data, size_t size)
    : type_(type), data_(std::move(data)), size_(size) {}

Type* Data::GetType(TypeTable* types) { return types->GetStrongPtrType(type_); }

CastExpr::CastExpr(Type* type, Expr* expr) : type_(type), expr_(expr) {}

Type* CastExpr::GetType(TypeTable* types) { return type_; }

EnumConstant::EnumConstant(const EnumValue* value) : value_(value) {}

Type* EnumConstant::GetType(TypeTable* types) { return value_->type; }

NullConstant::NullConstant() {}

Type* NullConstant::GetType(TypeTable* types) { return types->GetNull(); }

Stmts::Stmts() : scope_(nullptr) {}

void Stmts::AppendVar(std::shared_ptr<Var> var) { vars_.push_back(var); }

bool Stmts::ContainsReturn() const {
  for (auto stmt : stmts_) {
    if (stmt->ContainsReturn()) { return true; }
  }
  return false;
}

ExprList::ExprList(std::vector<Expr*> exprs) : exprs_(std::move(exprs)) {}

ExprStmt::ExprStmt(Expr* expr) : expr_(expr) {}

IfStatement::IfStatement(Expr* expr, Stmt* stmt, Stmt* optElse)
    : expr_(expr), stmt_(stmt), optElse_(optElse) {}

WhileStatement::WhileStatement(Expr* cond, Stmt* body) : cond_(cond), body_(body) {}

DoStatement::DoStatement(Stmt* body, Expr* cond) : body_(body), cond_(cond) {}

ForStatement::ForStatement(Stmt* initStmt, Expr* cond, Stmt* loopStmt, Stmt* body)
    : initStmt_(initStmt), cond_(cond), loopStmt_(loopStmt), body_(body) {}

ReturnStatement::ReturnStatement(Expr* expr, Scope* scope) : expr_(expr), scope_(scope) {}

NewArrayExpr::NewArrayExpr(Type* elementType, Expr* sizeExpr)
    : elementType_(elementType), sizeExpr_(sizeExpr) {}

Type* NewArrayExpr::GetType(TypeTable* types) {
  return types->GetStrongPtrType(types->GetArrayType(elementType_, 0, MemoryLayout::Default));
}

UnresolvedNewExpr::UnresolvedNewExpr(Type* type, Expr* length, ArgList* arglist)
    : type_(type), length_(length), arglist_(arglist) {}

Type* UnresolvedNewExpr::GetType(TypeTable* types) { return types->GetStrongPtrType(type_); }

NewExpr::NewExpr(Type* type, Expr* length, Method* constructor, ExprList* args)
    : type_(type), length_(length), constructor_(constructor), args_(args) {}

Type* NewExpr::GetType(TypeTable* types) { return types->GetStrongPtrType(type_); }

UnresolvedClassDefinition::UnresolvedClassDefinition(Scope* scope) : scope_(scope) {}

NodeVector::NodeVector() {}

Result AddressOf::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result Arg::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ArgList::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ArrayAccess::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result BinOpNode::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result BoolConstant::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result CastExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result Data::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result EnumConstant::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ExprList::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ConstructorNode::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result SmartToRawPtr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result DoStatement::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result DoubleConstant::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ExprStmt::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ExtractElementExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result FieldAccess::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result FloatConstant::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ForStatement::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result IfStatement::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result InsertElementExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result IntConstant::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result LengthExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result NewArrayExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result NewExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result NullConstant::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ReturnStatement::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result MethodCall::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result Stmts::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UIntConstant::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnaryOp::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedDot::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedIdentifier::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedListExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedClassDefinition::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedMethodCall::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedNewExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedStaticMethodCall::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedSwizzleExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result VarDeclaration::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result VarExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result LoadExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result IncDecExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ZeroInitStmt::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result StoreStmt::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result WhileStatement::Accept(Visitor* visitor) { return visitor->Visit(this); }
};  // namespace Toucan
