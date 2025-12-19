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

#include "symbol.h"

#include <string.h>
#include "type.h"

#include <ranges>

namespace Toucan {

SymbolTable::SymbolTable() {}

void SymbolTable::PushScope(Stmts* scope) {
  stack_.push_back(scope);
}

Stmts* SymbolTable::PopScope() {
  Stmts* back = PeekScope();
  stack_.pop_back();
  return back;
}

Stmts* SymbolTable::PeekScope() { return stack_.empty() ? nullptr : stack_.back(); }

Expr* SymbolTable::FindID(const std::string& identifier) const {
  for (auto scope : std::views::reverse(stack_)) {
    if (Expr* expr = scope->FindID(identifier)) return expr;
  }
  return nullptr;
}

void SymbolTable::DefineID(std::string identifier, Expr* expr) {
  assert(!stack_.empty());
  stack_.back()->DefineID(identifier, expr);
}

void SymbolTable::DefineType(std::string identifier, Type* type) {
  assert(!stack_.empty());
  stack_.back()->DefineType(identifier, type);
}

Type* SymbolTable::FindType(const std::string& identifier) const {
  for (auto scope : std::views::reverse(stack_)) {
    if (auto type = scope->FindType(identifier)) return type;
  }
  return nullptr;
}

};  // namespace Toucan
