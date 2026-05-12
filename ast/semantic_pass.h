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

#ifndef _AST_AST_SEMANTIC_PASS_H_
#define _AST_AST_SEMANTIC_PASS_H_

#include "copy_visitor.h"

#include <unordered_set>

namespace Toucan {

struct TypeLocationPair {
  Type*        type;
  FileLocation location;
};

using TypeLocationList = std::vector<TypeLocationPair>;

class SemanticPass : public CopyVisitor {
 public:
  SemanticPass(NodeVector* nodes, TypeTable* types);
  Stmts* Run(Stmts* stmts);
  Type*  ResolveType(ASTType* type);
  Result Visit(ASTArrayType* node) override;
  Result Visit(ASTAutoType* node) override;
  Result Visit(ASTBoolType* node) override;
  Result Visit(ASTClassType* node) override;
  Result Visit(ASTClassTemplateInstance* node) override;
  Result Visit(ASTEnumType* node) override;
  Result Visit(ASTFloatingPointType* node) override;
  Result Visit(ASTFormalTemplateArg* node) override;
  Result Visit(ASTIntegerType* node) override;
  Result Visit(ASTMatrixType* node) override;
  Result Visit(ASTQualifiedType* node) override;
  Result Visit(ASTRawPtrType* node) override;
  Result Visit(ASTScopedType* node) override;
  Result Visit(ASTStrongPtrType* node) override;
  Result Visit(ASTVectorType* node) override;
  Result Visit(ASTVoidType* node) override;
  Result Visit(ASTWeakPtrType* node) override;
  Result Visit(ArgList* node) override;
  Result Visit(ArrayAccess* node) override;
  Result Visit(BinOpNode* node) override;
  Result Visit(UnresolvedCastExpr* expr) override;
  Result Visit(ConstDecl* decl) override;
  Result Visit(ClassDecl* node) override;
  Result Visit(Data* expr) override;
  Result Visit(Decls* decls) override;
  Result Visit(DoStatement* stmt) override;
  Result Visit(EnumDecl* decls) override;
  Result Visit(ExprWithStmt* node) override;
  Result Visit(ForStatement* forStmt) override;
  Result Visit(IfStatement* stmt) override;
  Result Visit(LoadExpr* node) override;
  Result Visit(ReturnStatement* node) override;
  Result Visit(SliceExpr* node) override;
  Result Visit(SmartToRawPtr* expr) override;
  Result Visit(Stmts* stmts) override;
  Result Visit(IncDecExpr* node) override;
  Result Visit(StoreStmt* node) override;
  Result Visit(UnresolvedDot* node) override;
  Result Visit(UnresolvedIdentifier* node) override;
  Result Visit(UnresolvedInitializer* node) override;
  Result Visit(UnresolvedMethodCall* node) override;
  Result Visit(UnresolvedNewExpr* node) override;
  Result Visit(UnresolvedStaticDot* node) override;
  Result Visit(UnresolvedStaticMethodCall* node) override;
  Result Visit(VarDeclaration* decl) override;
  Result Visit(MethodDecl* decl) override;
  Result Visit(WhileStatement* stmt) override;
  Result Error(const char* fmt, ...);
  Result Default(ASTNode* node) override;
  int    GetNumErrors() const { return numErrors_; }

 private:
  void    UnwindStack(Stmts* stmts);
  Expr*   MakeConstantOne(Type* type);
  Expr*   MakeLoad(Expr* expr);
  Expr*   MakeReadOnlyTempVar(Expr* expr);
  Result  MakeSwizzle(int srcLength, Expr* expr, const std::string& id);
  Result  WideningError(Type* srcType, Type* dstType);
  Expr*   MakeSwizzleForStore(VectorType* lhsType, Expr* lhs, const std::string& id, Expr* rhs);
  Result  ResolveMethodCall(Expr* expr, ClassType* classType, std::string id, ArgList* arglist);
  Expr*   MakeDefaultInitializer(Type* type);
  void    AddDefaultInitializers(Type* type, std::vector<Expr*>* exprs);
  Expr*   ResolveListExpr(ArgList* argList, Type* dstType);
  void    WidenArgList(std::vector<Expr*>& argList, const VarVector& formalArgList);
  Expr*   Widen(Expr* expr, Type* dstType);
  Expr*   MakeIndexable(Expr* expr);
  Stmt*   Initialize(Expr* dest, Expr* initExpr = nullptr);
  Stmts*  InitializeClass(Expr* thisExpr, ClassType* classType);
  Stmts*  InitializeArray(Expr* dest, Type* elementType, Expr* length, ExprList* exprList);
  int     FindFormalArg(Arg* arg, Method* m, TypeTable* types);
  bool    MatchArgs(Expr* thisExpr, ArgList* args, Method* m, TypeTable* types, std::vector<Expr*>* newArgList);
  Expr*   FindID(std::string id);
  Expr*   AutoDereference(Expr* expr);
  Method* FindMethod(Expr*               thisExpr,
                     ClassType*          classType,
                     const std::string&  name,
                     ArgList*            args,
                     std::vector<Expr*>* newArgList);
  Method* FindOverriddenMethod(ClassType* classType, Method* method);
  bool             ResolveTypeList(ASTTypeList* typeList, TypeList* result);
  ClassType*       GetOrCreateClassType(ClassDecl* decl);
  Type*            PushQualifiers(Type* type, int qualifiers);
  void             SetCurrentTemplateArgs(const TypeList& srcTypes, const TypeList& dstTypes);

  ScopeStack       scopeStack_;
  TypeTable*       types_;
  TypeLocationList typesToValidate_;
  Stmts*           rootStmts_ = nullptr;
  int              numErrors_ = 0;
  Method*          currentMethod_ = nullptr;
  TypeMap          currentTemplateArgs_;
  std::unordered_set<std::string>  overloadedMethods_;
};

};  // namespace Toucan
#endif
