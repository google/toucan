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

ASTNode::ASTNode() {}

Expr::Expr() {}

HeapAllocation::HeapAllocation(Type* type, Expr* length) : type_(type), length_(length) {}

Type* HeapAllocation::GetType(TypeTable* types) {
  Type* type = type_;
  if (length_ != nullptr && !type_->IsUnsizedClass()) {
    type = types->GetArrayType(type_, 0, MemoryLayout::Default);
  }
  return types->GetRawPtrType(type);
}

Type* LoadExpr::GetType(TypeTable* types) {
  Type* type = expr_->GetType(types);
  assert(type->IsRawPtr());
  return static_cast<RawPtrType*>(type)->GetBaseType()->GetUnqualifiedType();
}

ExprWithStmt::ExprWithStmt(Expr* expr, Stmt* stmt) : expr_(expr), stmt_(stmt) {}

Type* ExprWithStmt::GetType(TypeTable* types) {
  return expr_->GetType(types);
}

Type* IncDecExpr::GetType(TypeTable* types) {
  Type* type = expr_->GetType(types);
  assert(type->IsRawPtr());
  return static_cast<RawPtrType*>(type)->GetBaseType()->GetUnqualifiedType();
}

Type* VarExpr::GetType(TypeTable* types) { return types->GetRawPtrType(var_->type); }

Type* TempVarExpr::GetType(TypeTable* types) {
  return types->GetRawPtrType(type_ ? type_ : initExpr_->GetType(types));
}

Type* SmartToRawPtr::GetType(TypeTable* types) {
  Type* type = expr_->GetType(types);
  assert(type->IsStrongPtr() || type->IsWeakPtr());
  return types->GetRawPtrType(static_cast<PtrType*>(type)->GetBaseType());
}

Type* RawToSmartPtr::GetType(TypeTable* types) {
  Type* type = expr_->GetType(types);
  assert(type->IsRawPtr());
  return types->GetStrongPtrType(static_cast<PtrType*>(type)->GetBaseType());
}

Type* ToRawArray::GetType(TypeTable* types) {
  auto type = types->GetArrayType(elementType_, 0, memoryLayout_);
  return types->GetRawPtrType(type);
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
  assert(type->IsRawPtr());
  type = static_cast<RawPtrType*>(type)->GetBaseType();
  int qualifiers;
  type = type->GetUnqualifiedType(&qualifiers);
  assert(type->IsArray());
  type = static_cast<ArrayType*>(type)->GetElementType();
  return types->GetRawPtrType(types->GetQualifiedType(type, qualifiers));
}

ASTNode::~ASTNode() {}

Arg::Arg(std::string id, Expr* expr, bool unfold) : id_(id), expr_(expr), unfold_(unfold) {}

ArgList::ArgList() {}

ArgList::ArgList(std::vector<Arg*>&& args) : args_(std::move(args)) {}

bool ArgList::IsNamed() const { return !args_.empty() && !args_.front()->GetID().empty(); }

BinOpNode::BinOpNode(Op op, Expr* lhs, Expr* rhs) : op_(op), lhs_(lhs), rhs_(rhs) {}

bool BinOpNode::IsRelOp() const {
  return op_ == LT || op_ == GT || op_ == LE || op_ == GE || op_ == EQ || op_ == NE;
}

Type* BinOpNode::GetType(TypeTable* types) {
  Type* lhsType = lhs_->GetType(types);
  Type* rhsType = rhs_->GetType(types);
  if (IsRelOp()) {
    if (lhsType->IsVector()) {
      return types->GetVector(types->GetBool(), static_cast<VectorType*>(lhsType)->GetNumElements());
    }
    return types->GetBool();
  } else {
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

bool UnaryOp::IsConstant(TypeTable* types) const {
  if (!rhs_->IsConstant(types)) return false;
  auto type = rhs_->GetType(types);
  return type->IsFloatingPoint() || type->IsInteger();
}

UnresolvedInitializer::UnresolvedInitializer(Type* type, ArgList* arglist, bool constructor)
    : type_(type), arglist_(arglist), constructor_(constructor) {}

Initializer::Initializer(Type* type, ExprList* arglist) : type_(type), arglist_(arglist) {}

UnresolvedListExpr::UnresolvedListExpr(ArgList* arglist) : arglist_(arglist) {}

Type* UnresolvedListExpr::GetType(TypeTable* types) {
  VarVector vars;
  for (auto arg : arglist_->GetArgs()) {
    vars.push_back(std::make_shared<Var>(arg->GetID(), arg->GetExpr()->GetType(types)));
  }
  return types->GetList(std::move(vars));
}

MethodDecl::MethodDecl(int modifiers, std::array<uint32_t, 3> workgroupSize, std::string id, Stmts* formalArguments, int thisQualifiers, Type* returnType, Expr* initializer, Stmts* body)
    : modifiers_(modifiers),
      id_(id),
      workgroupSize_(workgroupSize),
      formalArguments_(formalArguments),
      thisQualifiers_(thisQualifiers),
      returnType_(returnType),
      initializer_(initializer),
      body_(body) {}

Method* MethodDecl::CreateMethod(ClassType* classType, TypeTable* types) {
  Method* method = new Method(modifiers_, returnType_, id_, classType);
  method->stmts = body_;
  method->initializer = initializer_;
  method->workgroupSize = workgroupSize_;
  if (!(modifiers_ & Method::Modifier::Static)) {
    Type* thisType = types->GetQualifiedType(classType, thisQualifiers_);
    thisType = types->GetRawPtrType(thisType);
    method->AddFormalArg("this", thisType, nullptr);
  }
  if (formalArguments_) {
    for (auto& it : formalArguments_->GetStmts()) {
      VarDeclaration* v = static_cast<VarDeclaration*>(it);
      method->AddFormalArg(v->GetID(), v->GetType(), v->GetInitExpr());
    }
  }
  return method;
}

VarDeclaration::VarDeclaration(std::string id, Type* type, Expr* initExpr)
    : id_(id), type_(type), initExpr_(initExpr) {}

Decls::Decls() {}

ArrayAccess::ArrayAccess(Expr* expr, Expr* index) : expr_(expr), index_(index) {}

UnresolvedMethodCall::UnresolvedMethodCall(Expr* expr, std::string id, ArgList* arglist)
    : expr_(expr), id_(id), arglist_(arglist) {}

UnresolvedStaticMethodCall::UnresolvedStaticMethodCall(ClassType*  classType,
                                                       std::string id,
                                                       ArgList*    arglist)
    : classType_(classType), id_(id), arglist_(arglist) {}

MethodCall::MethodCall(Method* method, ExprList* arglist) : method_(method), arglist_(arglist) {}

VarExpr::VarExpr(Var* var) : var_(var) {}

TempVarExpr::TempVarExpr(Type* type, Expr* initExpr) : type_(type), initExpr_(initExpr) {}

LoadExpr::LoadExpr(Expr* expr) : expr_(expr) {}

StoreStmt::StoreStmt(Expr* lhs, Expr* rhs) : lhs_(lhs), rhs_(rhs) {}

DestroyStmt::DestroyStmt(Expr* expr) : expr_(expr) {}

IncDecExpr::IncDecExpr(Op op, Expr* expr, bool returnOrigValue)
    : op_(op), expr_(expr), returnOrigValue_(returnOrigValue) {}

SliceExpr::SliceExpr(Expr* expr, Expr* start, Expr* end) : expr_(expr), start_(start), end_(end) {}

Type* SliceExpr::GetType(TypeTable* types) {
  return expr_->GetType(types);
}

ZeroInitStmt::ZeroInitStmt(Expr* lhs) : lhs_(lhs) {}

UnresolvedDot::UnresolvedDot(Expr* expr, std::string id) : expr_(expr), id_(id) {}

UnresolvedStaticDot::UnresolvedStaticDot(Type* type, std::string id) : type_(type), id_(id) {}

SmartToRawPtr::SmartToRawPtr(Expr* expr) : expr_(expr) {}

RawToSmartPtr::RawToSmartPtr(Expr* expr) : expr_(expr) {}

ToRawArray::ToRawArray(Expr* data, Expr* length, Type* elementType, MemoryLayout memoryLayout) : data_(data), length_(length), elementType_(elementType), memoryLayout_(memoryLayout) {}

FieldAccess::FieldAccess(Expr* expr, Field* field) : expr_(expr), field_(field) {}

UnresolvedIdentifier::UnresolvedIdentifier(std::string id) : id_(id) {}

LengthExpr::LengthExpr(Expr* expr) : expr_(expr) {}

Type* LengthExpr::GetType(TypeTable* types) { return types->GetInt(); }

SwizzleExpr::SwizzleExpr(Expr* expr, const std::vector<int>& indices) : expr_(expr), indices_(indices) {}

Type* SwizzleExpr::GetType(TypeTable* types) {
  auto exprType = expr_->GetType(types);
  assert(exprType->IsVector());
  auto componentType = static_cast<VectorType*>(exprType)->GetElementType();
  auto size = indices_.size();
  if (size == 1) {
    return componentType;
  } else {
    return types->GetVector(componentType, indices_.size());
  }
}

ExtractElementExpr::ExtractElementExpr(Expr* expr, int index) : expr_(expr), index_(index) {}

Type* ExtractElementExpr::GetType(TypeTable* types) {
  Type* type = expr_->GetType(types)->GetUnqualifiedType();
  assert(type->IsVector());
  return static_cast<VectorType*>(type)->GetElementType();
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

bool CastExpr::IsTransparent(TypeTable* types) const {
  auto srcType = expr_->GetType(types);
  return (srcType->IsInt() && type_->IsUInt() ||
      srcType->IsUInt() && type_->IsInt() ||
      srcType->IsShort() && type_->IsUShort() ||
      srcType->IsUShort() && type_->IsShort() ||
      srcType->IsByte() && type_->IsUByte() ||
      srcType->IsUByte() && type_->IsByte());
}

Type* CastExpr::GetType(TypeTable* types) { return type_; }

EnumConstant::EnumConstant(const EnumValue* value) : value_(value) {}

Type* EnumConstant::GetType(TypeTable* types) { return value_->type; }

NullConstant::NullConstant() {}

Type* NullConstant::GetType(TypeTable* types) { return types->GetStrongPtrType(types->GetVoid()); }

Stmts::Stmts() {}

void Stmts::Append(const std::vector<Stmt*>& stmts) {
  stmts_.insert(stmts_.end(), stmts.begin(), stmts.end());
}

Expr* Stmts::FindID(const std::string& identifier) {
  auto i = ids_.find(identifier);
  if (i != ids_.end()) { return i->second; }
  return nullptr;
}

Type* Stmts::FindType(const std::string& identifier) {
  auto i = types_.find(identifier);
  if (i != types_.end()) { return i->second; }
  return nullptr;
}

void Stmts::AppendVar(std::shared_ptr<Var> var) { vars_.push_back(var); }

bool Stmts::ContainsReturn() const {
  for (auto stmt : stmts_) {
    if (stmt->ContainsReturn()) { return true; }
  }
  return false;
}

ExprList::ExprList() {}

ExprList::ExprList(std::vector<Expr*>&& exprs) : exprs_(std::move(exprs)) {}

bool ExprList::IsConstant(TypeTable* types) const {
  for (auto expr : exprs_) {
    if (!expr || !expr->IsConstant(types)) { return false; }
  }
  return true;
}

ExprStmt::ExprStmt(Expr* expr) : expr_(expr) {}

IfStatement::IfStatement(Expr* expr, Stmt* stmt, Stmt* optElse)
    : expr_(expr), stmt_(stmt), optElse_(optElse) {}

WhileStatement::WhileStatement(Expr* cond, Stmt* body) : cond_(cond), body_(body) {}

DoStatement::DoStatement(Stmt* body, Expr* cond) : body_(body), cond_(cond) {}

ForStatement::ForStatement(Stmt* initStmt, Expr* cond, Stmt* loopStmt, Stmt* body)
    : initStmt_(initStmt), cond_(cond), loopStmt_(loopStmt), body_(body) {}

ReturnStatement::ReturnStatement(Expr* expr) : expr_(expr) {}

UnresolvedNewExpr::UnresolvedNewExpr(Type* type, Expr* length, ArgList* arglist, bool constructor)
    : type_(type), length_(length), arglist_(arglist), constructor_(constructor) {}

Type* UnresolvedNewExpr::GetType(TypeTable* types) { return types->GetStrongPtrType(type_); }

UnresolvedClassDefinition::UnresolvedClassDefinition(ClassType* classType) : class_(classType) {}

NodeVector::NodeVector() {}

Result Arg::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ArgList::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ArrayAccess::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result BinOpNode::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result BoolConstant::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result CastExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result Data::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result EnumConstant::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ExprList::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ExprWithStmt::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result SmartToRawPtr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result RawToSmartPtr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ToRawArray::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result DoStatement::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result DoubleConstant::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ExprStmt::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ExtractElementExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result FieldAccess::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result FloatConstant::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ForStatement::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result HeapAllocation::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result IfStatement::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result Initializer::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result InsertElementExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result IntConstant::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result LengthExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result MethodDecl::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result NullConstant::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ReturnStatement::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result MethodCall::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result Stmts::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result SwizzleExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result TempVarExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UIntConstant::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnaryOp::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result DestroyStmt::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedDot::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedIdentifier::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedListExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedClassDefinition::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedInitializer::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedMethodCall::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedNewExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedStaticDot::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result UnresolvedStaticMethodCall::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result VarDeclaration::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result Decls::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result VarExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result LoadExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result IncDecExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result SliceExpr::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result ZeroInitStmt::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result StoreStmt::Accept(Visitor* visitor) { return visitor->Visit(this); }
Result WhileStatement::Accept(Visitor* visitor) { return visitor->Visit(this); }
};  // namespace Toucan
