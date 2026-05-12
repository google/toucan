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

#include <string>

#include <unordered_set>

#include "ast.h"

namespace Toucan {

class ClassDecl;
class MethodDecl;

class OverloadFinder : public Visitor {
 public:
  OverloadFinder() {}
  Result Visit(MethodDecl* node) override {
    auto name = node->GetID();
    if (methodNames_.contains(name)) {
      overloadedMethodNames_.insert(name);
    } else {
      methodNames_.insert(name);
    }
    return {};
  }
  Result Default(ASTNode* node) override {
    return {};
  }
  static std::unordered_set<std::string> Run(Decls* node) {
    OverloadFinder finder;
    for (auto decl : node->Get()) {
      decl->Accept(&finder);
    }
    return std::move(finder.overloadedMethodNames_);
  }
 private:
  std::unordered_set<std::string> methodNames_;
  std::unordered_set<std::string> overloadedMethodNames_;
};

std::string GetMangledName(ClassDecl* classDecl, MethodDecl* method, bool overloaded);

}
