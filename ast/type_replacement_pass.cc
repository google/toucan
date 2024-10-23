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

#include "type_replacement_pass.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "symbol.h"

namespace Toucan {

TypeReplacementPass::TypeReplacementPass(NodeVector*     nodes,
                                         SymbolTable*    symbols,
                                         TypeTable*      types,
                                         const TypeList& srcTypes,
                                         const TypeList& dstTypes)
    : CopyVisitor(nodes),
      symbols_(symbols),
      types_(types),
      srcTypes_(srcTypes),
      dstTypes_(dstTypes) {}

Method* TypeReplacementPass::ResolveMethod(Method* m) {
  Method* result = new Method(m->modifiers, ResolveType(m->returnType), m->name.c_str(),
                              static_cast<ClassType*>(ResolveType(m->classType)));
  result->templateMethod = m;
  for (int i = 0; i < m->formalArgList.size(); ++i) {
    Var* var = m->formalArgList[i].get();
    result->AddFormalArg(var->name, ResolveType(var->type), m->defaultArgs[i]);
  }
  if (m->stmts) {
    result->stmts = Resolve(m->stmts);
    if (result->stmts->GetScope()) { result->stmts->GetScope()->method = result; }
  }
  return result;
}

void TypeReplacementPass::ResolveClassInstance(ClassTemplate* classTemplate, ClassType* instance) {
  srcTypes_.push_back(classTemplate);
  dstTypes_.push_back(instance);
  instance->SetScope(symbols_->PushNewScope());
  for (const auto& i : classTemplate->GetScope()->types) {
    instance->GetScope()->types[i.first] = ResolveType(i.second);
  }
  for (const auto& i : classTemplate->GetMethods()) {
    Method* method = i.get();
    instance->AddMethod(ResolveMethod(method));
  }
  for (const auto& i : classTemplate->GetFields()) {
    Field* field = i.get();
    instance->AddField(field->name, ResolveType(field->type), Resolve(field->defaultValue));
  }
  symbols_->PopScope();
}

Type* TypeReplacementPass::PushQualifiers(Type* type, int qualifiers) {
  if (type->IsStrongPtr()) {
    return types_->GetStrongPtrType(
        PushQualifiers(static_cast<PtrType*>(type)->GetBaseType(), qualifiers));
  } else if (type->IsWeakPtr()) {
    return types_->GetWeakPtrType(
        PushQualifiers(static_cast<PtrType*>(type)->GetBaseType(), qualifiers));
  } else if (type->IsArray()) {
    auto arrayType = static_cast<ArrayType*>(type);
    return types_->GetArrayType(
        PushQualifiers(arrayType->GetElementType(), qualifiers),
        arrayType->GetNumElements(), arrayType->GetMemoryLayout());
  } else {
    return types_->GetQualifiedType(type, qualifiers);
  }
}

Type* TypeReplacementPass::ResolveType(Type* type) {
  if (!type) { return nullptr; }
  for (int i = 0; i < srcTypes_.size(); i++) {
    if (type == srcTypes_[i]) { return dstTypes_[i]; }
  }
  if (type->IsArray()) {
    ArrayType* atype = static_cast<ArrayType*>(type);
    return types_->GetArrayType(ResolveType(atype->GetElementType()), atype->GetNumElements(),
                                atype->GetMemoryLayout());
  } else if (type->IsVector()) {
    VectorType* vtype = static_cast<VectorType*>(type);
    return types_->GetVector(ResolveType(vtype->GetComponentType()), vtype->GetLength());
  } else if (type->IsMatrix()) {
    MatrixType* mtype = static_cast<MatrixType*>(type);
    VectorType* columnType = static_cast<VectorType*>(ResolveType(mtype->GetColumnType()));
    assert(columnType->IsVector());
    return types_->GetMatrix(columnType, mtype->GetNumColumns());
  } else if (type->IsStrongPtr()) {
    return types_->GetStrongPtrType(ResolveType(static_cast<PtrType*>(type)->GetBaseType()));
  } else if (type->IsWeakPtr()) {
    return types_->GetWeakPtrType(ResolveType(static_cast<PtrType*>(type)->GetBaseType()));
  } else if (type->IsQualified()) {
    int   qualifiers;
    Type* result = ResolveType(type->GetUnqualifiedType(&qualifiers));
    result = PushQualifiers(result, qualifiers);
    return result;
  } else if (type->IsClass() && static_cast<ClassType*>(type)->GetTemplate()) {
    ClassType* instance = static_cast<ClassType*>(type);
    TypeList   newArgs;
    for (Type* const& arg : instance->GetTemplateArgs()) {
      newArgs.push_back(ResolveType(arg));
    }
    return types_->GetClassTemplateInstance(instance->GetTemplate(), newArgs);
  } else if (type->IsUnresolvedScopedType()) {
    auto  ust = static_cast<UnresolvedScopedType*>(type);
    Type* newType = ResolveType(ust->GetBaseType());
    if (ust->GetID() == "ElementType") {
      if (newType->IsArray()) {
        newType = static_cast<ArrayType*>(newType)->GetElementType();
      } else {
        newType = types_->GetVoid();
      }
    } else if (newType->IsClass()) {
      auto classType = static_cast<ClassType*>(newType);
      if (ust->GetID() == "BaseClass") {
        if (auto parent = classType->GetParent()) { newType = parent; }
      } else {
        newType = static_cast<ClassType*>(newType)->FindType(ust->GetID());
        if (!newType) {
          Error("Type \"%s\" not found in \"%s\"", ust->GetID().c_str(),
                newType->ToString().c_str());
        }
      }
    } else {
      Error("\"%s\" is not a class", newType->ToString().c_str());
    }
    return newType;
  }
  return type;
}

TypeList* TypeReplacementPass::ResolveTypes(TypeList* typeList) {
  if (!typeList) { return nullptr; }
  TypeList* list = new TypeList();  // FIXME: who owns this?
  for (Type* const& type : *typeList) {
    list->push_back(ResolveType(type));
  }
  return list;
}

Result TypeReplacementPass::Visit(Stmts* stmts) {
  Stmts* newStmts = Make<Stmts>();
  newStmts->SetFileLocation(stmts->GetFileLocation());
  if (stmts->GetScope()) { newStmts->SetScope(symbols_->PushNewScope()); }

  for (Stmt* const& it : stmts->GetStmts()) {
    Stmt* stmt = Resolve(it);
    if (stmt) newStmts->Append(stmt);
  }
  if (stmts->GetScope()) { symbols_->PopScope(); }
  return newStmts;
}

Result TypeReplacementPass::Default(ASTNode* node) {
  assert(false);
  return nullptr;
}

Result TypeReplacementPass::Error(const char* fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  fprintf(stderr, "\n");
  numErrors_++;
  return nullptr;
}

};  // namespace Toucan
