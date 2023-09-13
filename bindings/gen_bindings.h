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

#ifndef _BINDINGS_GEN_BINDINGS_H_
#define _BINDINGS_GEN_BINDINGS_H_

#include <stdio.h>
#include <unordered_map>

#include <ast/dump_as_source_pass.h>

namespace Toucan {

class SymbolTable;
class TypeTable;
class ClassType;
class EnumType;
class Method;
class Symbol;
class Type;

class GenBindings {
 public:
  GenBindings(SymbolTable* symbols,
              TypeTable*   types,
              FILE*        file,
              FILE*        header,
              bool         dumpStmtsAsSource);
  void Run();
  void GenType(Type* type);
  void GenBindingsForClass(ClassType* classType);
  void GenBindingsForEnum(EnumType* enumType);
  void GenBindingsForMethod(ClassType* classType, Method* method);

 private:
  SymbolTable*                   symbols_;
  TypeTable*                     types_;
  FILE*                          file_;
  FILE*                          header_;
  bool                           dumpStmtsAsSource_;
  std::unordered_map<Type*, int> typeMap_;
  DumpAsSourcePass               sourcePass_;
};

};  // namespace Toucan
#endif
