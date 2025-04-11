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

#ifndef _AST_SYMBOL_H_
#define _AST_SYMBOL_H_

#include <string>
#include <unordered_map>
#include <vector>

#ifndef _AST_TYPE_H
#include "type.h"
#endif

namespace Toucan {

typedef std::unordered_map<std::string, std::shared_ptr<Var>> VarMap;
typedef std::unordered_map<std::string, Type*>                TypeMap;

struct Scope {
  Scope(Scope* p) : parent(p), classType(nullptr), enumType(nullptr), method(nullptr) {}
  Scope*     parent;
  ClassType* classType;
  EnumType*  enumType;
  Method*    method;
  VarMap     vars;
  TypeMap    types;
};

class SymbolTable {
 public:
  SymbolTable();
  Var*             FindVar(const std::string& identifier) const;
  Type*            FindType(const std::string& identifier) const;
  Field*           FindField(const std::string& identifier) const;
  Var*             FindVarInScope(const std::string& identifier) const;
  Var*             DefineVar(std::string identifier, Type* type);
  bool             DefineType(std::string identifier, Type* type);
  Scope*           PushNewScope();
  void             PushScope(Scope* scope);
  Scope*           PopScope();
  Scope*           PeekScope();
  void             Dump();

 private:
  Scope*                              currentScope_;
  std::vector<std::unique_ptr<Scope>> scopes_;
};

};  // namespace Toucan

#endif
