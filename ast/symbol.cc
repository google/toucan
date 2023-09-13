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

namespace Toucan {

SymbolTable::SymbolTable() : currentScope_(nullptr) {}

Scope* SymbolTable::PushNewScope() {
  Scope* scope = new Scope(currentScope_);
  scopes_.push_back(std::unique_ptr<Scope>(scope));
  currentScope_ = scope;
  return scope;
}

void SymbolTable::PushScope(Scope* scope) { currentScope_ = scope; }

Scope* SymbolTable::PopScope() {
  Scope* back = currentScope_;
  currentScope_ = back ? back->parent : nullptr;
  return back;
}

Scope* SymbolTable::PeekScope() { return currentScope_; }

Var* SymbolTable::FindVar(const std::string& identifier) const {
  Scope* scope = currentScope_;
  for (scope = currentScope_; scope != nullptr; scope = scope->parent) {
    VarMap::const_iterator j = scope->vars.find(identifier);
    if (j != scope->vars.end()) { return j->second.get(); }
    if (scope->method) {
      for (const auto& it : scope->method->formalArgList) {
        Var* var = it.get();
        if (var->name == identifier) { return var; }
      }
    }
  }
  return 0;
}

Field* SymbolTable::FindField(const std::string& identifier) const {
  Scope* scope = currentScope_;
  for (scope = currentScope_; scope != nullptr; scope = scope->parent) {
    ClassType* classType = scope->classType;
    if (classType) { return classType->FindField(identifier); }
  }
  return nullptr;
}

Var* SymbolTable::FindVarInScope(const std::string& identifier) const {
  if (!currentScope_) return nullptr;
  VarMap::const_iterator j = currentScope_->vars.find(identifier);
  if (j != currentScope_->vars.end()) { return j->second.get(); }
  return 0;
}

Var* SymbolTable::DefineVar(std::string identifier, Type* type) {
  if (!currentScope_) return nullptr;
  auto var = std::make_shared<Var>(identifier, type);
  currentScope_->vars[identifier] = var;
  return var.get();
}

bool SymbolTable::DefineType(std::string identifier, Type* type) {
  if (!currentScope_) return false;
  currentScope_->types[identifier] = type;
  return true;
}

Type* SymbolTable::FindType(const std::string& identifier) const {
  Scope* scope = currentScope_;
  for (scope = currentScope_; scope != nullptr; scope = scope->parent) {
    TypeMap::const_iterator j = scope->types.find(identifier);
    if (j != scope->types.end()) { return j->second; }
  }
  return 0;
}

const EnumValue* SymbolTable::FindEnumValue(const std::string& identifier) const {
  Scope* scope = currentScope_;
  for (scope = currentScope_; scope != nullptr; scope = scope->parent) {
    for (const auto& j : scope->types) {
      Type* type = j.second;
      if (type->IsEnum()) {
        for (const EnumValue& k : static_cast<EnumType*>(type)->GetValues()) {
          if (identifier == k.id) { return &k; }
        }
      }
    }
  }
  return 0;
}

void SymbolTable::Dump() {
  for (const auto& scope : scopes_) {
    printf("Scope%s:\n", scope->method ? " (method)" : "");
    for (auto p : scope->vars) {
      Var* v = p.second.get();
      printf("  %s", v->type->ToString().c_str());
      printf(" %s;\n", v->name.c_str());
    }
    for (const auto& i : scope->types) {
      const char* name = i.first.c_str();
      const Type* t = i.second;
      if (t->IsClass()) {
        printf("  class %s;\n", name);
      } else if (t->IsEnum()) {
        printf("  enum %s;\n", name);
      }
    }
    if (scope->method) { printf("TODO:  print formal args here\n"); }
  }
}

};  // namespace Toucan
