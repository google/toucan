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

#include "ast.h"
#ifndef _AST_TYPE_H
#include "type.h"
#endif

namespace Toucan {

class SymbolTable {
 public:
  SymbolTable();
  Expr*            FindID(const std::string& identifier) const;
  Type*            FindType(const std::string& identifier) const;
  void             DefineID(std::string identifier, Expr* expr);
  void             DefineType(std::string identifier, Type* type);
  void             PushScope(Stmts* scope);
  Stmts*           PopScope();
  Stmts*           PeekScope();
  const std::vector<Stmts*>& Get() { return stack_; }

 private:
  std::vector<Stmts*>  stack_;
};

};  // namespace Toucan

#endif
