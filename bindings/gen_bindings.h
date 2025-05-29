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

#include <list>
#include <ostream>
#include <unordered_map>

#include "dump_as_source_pass.h"

namespace Toucan {

class ClassType;
class Method;
class Type;

class GenBindings {
 public:
  GenBindings(std::ostream&  file,
              std::ostream&  header,
              bool           emitScopesAndStatements);
  void Run(const TypeVector& referencedTypes);
  int  EmitType(Type* type);

 private:
  void EmitClass(ClassType* classType);
  void EmitMethod(Method* method);

 private:
  SymbolTable*                   symbols_;
  TypeTable*                     types_;
  TypeVector                     referencedTypes_;
  std::list<ClassType*>          classes_;
  std::ostream&                  file_;
  std::ostream&                  header_;
  bool                           emitScopesAndStatements_;
  std::unordered_map<Type*, int> typeMap_;
  int                            numTypes_ = 0;
  DumpAsSourcePass               sourcePass_;
};

};  // namespace Toucan
#endif
