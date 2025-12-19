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

#ifndef _AST_TYPE_REPLACEMENT_PASS_H_
#define _AST_TYPE_REPLACEMENT_PASS_H_

#include <queue>

#include "copy_visitor.h"

namespace Toucan {

class SymbolTable;

class TypeReplacementPass : public CopyVisitor {
 public:
  TypeReplacementPass(NodeVector*     nodes,
                      TypeTable*      types,
                      const TypeList& srcTypes,
                      const TypeList& dstTypes,
                      std::queue<ClassType*>* instanceQueue);
  Result    Error(const char* fmt, ...);
  Type*     ResolveType(Type* type) override;
  TypeList* ResolveTypes(TypeList* typeList);
  Method*   ResolveMethod(Method* method);
  void      ResolveClassInstance(ClassTemplate* classTemplate, ClassType* instance);
  Result    Visit(Stmts* stmts) override;
  Result    Default(ASTNode* node) override;
  int       NumErrors() const { return numErrors_; }

 private:
  Type*        PushQualifiers(Type* type, int qualifiers);
  TypeTable*   types_;
  TypeList     srcTypes_;
  TypeList     dstTypes_;
  std::queue<ClassType*>* instanceQueue_;
  int          numErrors_ = 0;
};

};  // namespace Toucan
#endif
