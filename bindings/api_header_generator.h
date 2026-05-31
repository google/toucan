// Copyright 2026 The Toucan Authors
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

#ifndef _BINDINGS_API_HEADER_GENERATOR_H_
#define _BINDINGS_API_HEADER_GENERATOR_H_

#include <ast/ast.h>

#include <unordered_set>

namespace Toucan {

class APIHeaderGenerator : public Visitor {
 public:
                APIHeaderGenerator(Stmts* stmts, TypeTable* types, std::ostream& header);
  void          Run();
  Result        Visit(ASTArrayType* node) override;
  Result        Visit(ASTAutoType* node) override;
  Result        Visit(ASTBoolType* node) override;
  Result        Visit(ASTClassTemplateInstance* node) override;
  Result        Visit(ASTClassType* node) override;
  Result        Visit(ASTEnumType* node) override;
  Result        Visit(ASTEnumValue* node) override;
  Result        Visit(ASTFloatingPointType* node) override;
  Result        Visit(ASTFormalTemplateArg* node) override;
  Result        Visit(ASTIntegerType* node) override;
  Result        Visit(ASTMatrixType* node) override;
  Result        Visit(ASTQualifiedType* node) override;
  Result        Visit(ASTRawPtrType* node) override;
  Result        Visit(ASTStrongPtrType* node) override;
  Result        Visit(ASTVectorType* node) override;
  Result        Visit(ASTVoidType* node) override;
  Result        Visit(BoolConstant* constant) override;
  Result        Visit(ClassDecl* node) override;
  Result        Visit(ClassTemplateDecl* node) override;
  Result        Visit(Decls* decls) override;
  Result        Visit(DoubleConstant* constant) override;
  Result        Visit(EnumDecl* node) override;
  Result        Visit(FloatConstant* constant) override;
  Result        Visit(IntConstant* constant) override;
  Result        Visit(ReturnStatement* stmt) override;
  Result        Visit(LoadExpr* node) override;
  Result        Visit(Stmts* stmts) override;
  Result        Visit(TempVarExpr* node) override;
  Result        Visit(UIntConstant* constant) override;
  Result        Visit(UnresolvedInitializer* node) override;
  Result        Visit(UnresolvedStaticDot* node) override;
  Result        Visit(VarDeclaration* node) override;
  Result        Visit(ConstDecl* node) override;
  Result        Visit(MethodDecl* node) override;
  Result        Default(ASTNode* node) override;
  void          WriteCurrentVarID();
  void          WriteCurrentIndices();

 private:
  Stmts*                           stmts_;
  TypeTable*                       types_;
  std::ostream&                    header_;
  bool                             emitMethods_ = false;
  bool                             pointerContext_ = false;
  bool                             rawPointerContext_ = false;
  ClassDecl*                       currentClassDecl_ = nullptr;
  MethodDecl*                      currentMethodDecl_ = nullptr;
  Expr*                            currentAutoExpr_ = nullptr;
  std::string                      currentVarID_;
  std::vector<int>                 currentIndices_;
  std::unordered_set<std::string>  overloadedMethodNames_;
};

};  // namespace Toucan
#endif
