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

#include "gen_bindings.h"

#include <cassert>
#include <iostream>

namespace Toucan {

namespace {

const char* MemoryLayoutToString(MemoryLayout layout) {
  switch (layout) {
    case MemoryLayout::Default: return "Default";
    case MemoryLayout::Storage: return "Storage";
    case MemoryLayout::Uniform: return "Uniform";
    default: assert(!"unknown MemoryLayout"); return "";
  }
}

std::string ConvertType(Type* type, const std::string& str) {
  if (type->IsPtr()) {
    return ConvertType(static_cast<PtrType*>(type)->GetBaseType(), "*" + str);
  } else if (type->IsArrayLike()) {
    ArrayLikeType* a = static_cast<VectorType*>(type);
    return ConvertType(a->GetElementType(), str + "[" + std::to_string(a->GetNumElements()) + "]");
  } else if (type->IsClass()) {
    return static_cast<ClassType*>(type)->GetName() + " " + str;
  } else if (type->IsByte()) {
    return "int8_t " + str;
  } else if (type->IsUByte()) {
    return "uint8_t " + str;
  } else if (type->IsShort()) {
    return "int16_t " + str;
  } else if (type->IsUShort()) {
    return "uint16_t " + str;
  } else if (type->IsInt()) {
    return "int32_t " + str;
  } else if (type->IsUInt()) {
    return "uint32_t " + str;
  } else if (type->IsEnum()) {
    return static_cast<EnumType*>(type)->GetName() + " " + str;
  } else if (type->IsFloat()) {
    return "float " + str;
  } else if (type->IsDouble()) {
    return "double " + str;
  } else if (type->IsBool()) {
    return "bool " + str;
  } else if (type->IsVoid()) {
    return "void " + str;
  } else if (type->IsQualified()) {
    return ConvertType(static_cast<QualifiedType*>(type)->GetBaseType(), str);
  } else {
    assert(!"ConvertType:  unknown type");
    return 0;
  }
}

}  // namespace

GenBindings::GenBindings(std::ostream& file,
                         std::ostream& header,
                         bool          emitSymbolsAndStatements)
    : file_(file),
      header_(header),
      emitSymbolsAndStatements_(emitSymbolsAndStatements),
      sourcePass_(file_, this) {}

int GenBindings::EmitType(Type* type) {
  if (!type) return -1;
  auto iter = typeMap_.find(type);
  if (iter != typeMap_.end()) {
    return iter->second;
  }
  int id = -1;
  auto emitLHS = [this, type, &id]() -> std::ostream& {
    id = numTypes_++;
    typeMap_[type] = id;
    file_ << "  auto* type" << id << " = ";
    return file_;
  };
  if (type->IsInteger()) {
    IntegerType* i = static_cast<IntegerType*>(type);
    emitLHS() << "types->GetInteger(" << std::to_string(i->GetBits()) << ", "
              << (i->Signed() ? "true" : "false") << ");\n";
  } else if (type->IsFloatingPoint()) {
    FloatingPointType* f = static_cast<FloatingPointType*>(type);
    emitLHS() << "types->GetFloatingPoint(" << std::to_string(f->GetBits()) << ");\n";
  } else if (type->IsBool()) {
    emitLHS() << "types->GetBool();\n";
  } else if (type->IsVector()) {
    VectorType* v = static_cast<VectorType*>(type);
    auto componentTypeID = EmitType(v->GetElementType());
    emitLHS() << "types->GetVector(type" << componentTypeID << ", "
              << v->GetNumElements() << ");\n";
  } else if (type->IsMatrix()) {
    MatrixType* m = static_cast<MatrixType*>(type);
    auto columnTypeID = EmitType(m->GetColumnType());
    emitLHS() << "types->GetMatrix(type" << columnTypeID << ", " << m->GetNumColumns()
              << ");\n";
  } else if (type->IsString()) {
    emitLHS() << "types->GetString();\n";
  } else if (type->IsVoid()) {
    emitLHS() << "types->GetVoid();\n";
  } else if (type->IsAuto()) {
    emitLHS() << "types->GetAuto();\n";
  } else if (type->IsClassTemplate()) {
    ClassTemplate* classTemplate = static_cast<ClassTemplate*>(type);
    emitLHS() << "types->Make<ClassTemplate>(\"" << classTemplate->GetName()
              << "\", TypeList({";
    for (Type* const& type : classTemplate->GetFormalTemplateArgs()) {
      assert(type->IsFormalTemplateArg());
      file_ << "types->GetFormalTemplateArg(\""
            << static_cast<FormalTemplateArg*>(type)->GetName() << "\")";
      if (&type != &classTemplate->GetFormalTemplateArgs().back()) file_ << ", ";
    }
    if (header_) {
      header_ << "struct " << classTemplate->GetName() << ";\n";
    }
    file_ << "}));\n";
    classes_.push_back(classTemplate);
  } else if (type->IsClass()) {
    ClassType* classType = static_cast<ClassType*>(type);
    if (classType->GetTemplate()) {
      int templateID = EmitType(classType->GetTemplate());
      std::vector<int> args;
      for (auto type : classType->GetTemplateArgs()) {
        args.push_back(EmitType(type));
      }
      emitLHS() << "types->GetClassTemplateInstance(type" << templateID << ", {";
      for (auto arg : args) {
        file_ << "type" << arg;
        if (&arg != &args.back()) { file_ << ", "; }
      }
      file_ << "}, nullptr);\n";
    } else {
      emitLHS() << "types->Make<ClassType>(\"" << classType->GetName() << "\");\n";
      if (header_) {
        int pad = 0;
        if (classType->GetFields().size() > 0) {
          classType->ComputeFieldOffsets();
          header_ << "struct " << classType->GetName() << " {\n";
          for (const auto& field : classType->GetFields()) {
            header_ << "  " << ConvertType(field->type, field->name) << ";\n";
            if (field->padding > 0) {
              header_ << "  uint8_t pad" << pad++ << "[" << field->padding << "];\n";
            }
          }
          header_ << "};\n";
        } else {
          header_ << "struct " << classType->GetName() << ";\n";
        }
      }
    }
    classes_.push_back(classType);
  } else if (type->IsEnum()) {
    EnumType* enumType = static_cast<EnumType*>(type);
    emitLHS() << "types->Make<EnumType>(\"" << enumType->GetName() << "\");\n";
    if (header_) {
      header_ << "enum class " << enumType->GetName() << " {\n";
      const EnumValueVector& values = enumType->GetValues();
      for (EnumValue const& v : values) {
        header_ << "  " << v.id << " = " << v.value << ",\n";
      }
      header_ << "};\n";
    }
    for (const EnumValue& v : enumType->GetValues()) {
      file_ << "  type" << id << "->Append(\"" << v.id << "\", " << v.value << ");\n";
    }
    if (emitSymbolsAndStatements_) {
      file_ << "  rootStmts->DefineType(\"" << enumType->GetName() << "\", type" << id << ");\n";
    }
  } else if (type->IsPtr()) {
    PtrType* ptrType = static_cast<PtrType*>(type);
    std::string baseType = "type" + std::to_string(EmitType(ptrType->GetBaseType()));
    emitLHS() << "types->Get"
              << (type->IsStrongPtr() ? "Strong" : type->IsWeakPtr() ? "Weak" : "Raw")
              << "PtrType(" << baseType << ");\n";
  } else if (type->IsArray()) {
    ArrayType* arrayType = static_cast<ArrayType*>(type);
    auto elementTypeID = EmitType(arrayType->GetElementType());
    emitLHS() << "types->GetArrayType(type" << elementTypeID << ", "
              << arrayType->GetNumElements() << ", MemoryLayout::"
              << MemoryLayoutToString(arrayType->GetMemoryLayout()) << ");\n";
  } else if (type->IsFormalTemplateArg()) {
    FormalTemplateArg* formalTemplateArg = static_cast<FormalTemplateArg*>(type);
    emitLHS() << "types->GetFormalTemplateArg(\"" << formalTemplateArg->GetName()
              << "\");\n";
  } else if (type->IsQualified()) {
    QualifiedType* qualifiedType = static_cast<QualifiedType*>(type);
    int baseTypeID = EmitType(qualifiedType->GetBaseType());
    emitLHS() << "types->GetQualifiedType(type" << baseTypeID << ", "
              << qualifiedType->GetQualifiers() << ");\n";
  } else if (type->IsUnresolvedScopedType()) {
    auto unresolvedScopedType = static_cast<UnresolvedScopedType*>(type);
    auto baseTypeID = EmitType(unresolvedScopedType->GetBaseType());
    emitLHS() << "types->GetUnresolvedScopedType(type" << baseTypeID << ", \""
              << unresolvedScopedType->GetID() << "\");\n";
  } else if (type->IsList()) {
    const VarVector& vars = static_cast<ListType*>(type)->GetTypes();
    std::vector<int> varIDs;
    for (auto var : vars) {
      varIDs.push_back(EmitType(var->type));
    }
    emitLHS() << "types->GetList(VarVector{";
    int i = 0;
    for (auto var : vars) {
      file_ << "std::make_shared<Var>(\"" << var->name << "\", type" << varIDs[i++] << ")";
      if (var != vars.back()) file_ << ", ";
    }
    file_ << "});\n";
  } else {
    assert(!"unknown type");
    exit(-1);
  }
  return id;
}

void GenBindings::Run(const TypeVector& referencedTypes) {
  file_ << "#include <cstdint>\n";
  file_ << "#include <ast/ast.h>\n";
  file_ << "#include <ast/native_class.h>\n";
  file_ << "#include <ast/type.h>\n";
  file_ << "\n";
  file_ << "namespace Toucan {\n\n";
  if (emitSymbolsAndStatements_) {
    file_ << "void InitAPI(NodeVector* nodes, TypeTable* types, Stmts* rootStmts) {\n";
  } else {
    file_ << "Type** InitTypes(TypeTable* types) {\n";
  }
  if (referencedTypes.empty()) {
    file_ << "  return nullptr;\n"
          << "}\n}\n";
    return;
  }
  file_ << "  ClassType* c;\n";
  file_ << "  Method *m;\n";
  file_ << "\n";
  if (header_) {
    header_ << "#include <cstdint>\n";
    header_ << "extern \"C\" {\n";
    header_ << "namespace Toucan {\n\n";
    header_ << "class ClassType;\n";
    header_ << "class Type;\n";
    header_ << "using Deleter = void(*)(void*);\n\n";
    header_ << "struct ControlBlock {\n";
    header_ << "  uint32_t    strongRefs = 0;\n";
    header_ << "  uint32_t    weakRefs = 0;\n";
    header_ << "  uint32_t    arrayLength;\n";
    header_ << "  Type*       type = nullptr;\n";
    header_ << "  Deleter     deleter = nullptr;\n";
    header_ << "};\n\n";
    header_ << "struct Object {\n";
    header_ << "  void*          ptr;\n";
    header_ << "  ControlBlock  *controlBlock;\n";
    header_ << "};\n\n";
    header_ << "struct Array {\n";
    header_ << "  void*          ptr;\n";
    header_ << "  uint32_t       length;\n";
    header_ << "};\n\n";
  }
  for (auto type : referencedTypes) {
    EmitType(type);
  }
  while (!classes_.empty()) {
    auto classType = classes_.front();
    classes_.pop_front();
    EmitClass(classType);
  }
  if (!emitSymbolsAndStatements_) {
    file_ << "  static Type* typeList[" << referencedTypes.size() << "];\n\n";
    int i = 0;
    for (auto type : referencedTypes) {
      file_ << "  typeList[" << i++ << "] = type" << typeMap_[type] << ";\n";
    }
    file_ << "  return typeList;\n";
  }
  file_ << "}\n\n";
  file_ << "};\n";
  if (header_) {
    header_ << "\n};\n}\n";
  }
  typeMap_.clear();
  numTypes_ = 0;
}

void PrintNativeType(std::ostream& result, Type* type) {
  if (type->IsVoid()) {
    result << "void";
  } else if (type->IsInteger()) {
    IntegerType* integerType = static_cast<IntegerType*>(type);
    result << (integerType->Signed() ? "int" : "uint") << integerType->GetBits() << "_t";
  } else if (type->IsBool()) {
    result << "bool";
  } else if (type->IsFloat()) {
    result << "float";
  } else if (type->IsDouble()) {
    result << "double";
  } else if (type->IsClass()) {
    result << static_cast<ClassType*>(type)->GetName();
  } else if (type->IsEnum()) {
    result << static_cast<EnumType*>(type)->GetName();
  } else if (type->IsRawPtr()) {
    auto baseType = static_cast<RawPtrType*>(type)->GetBaseType();
    baseType = baseType->GetUnqualifiedType();
    if (baseType->IsClass()) {
      PrintNativeType(result, baseType);
      result << "*";
    } else if (baseType->IsUnsizedArray()) {
      result << "Array*";
    } else {
      result << "void*";
    }
  } else if (type->IsStrongPtr() || type->IsWeakPtr()) {
    Type* baseType = static_cast<PtrType*>(type)->GetBaseType()->GetUnqualifiedType();
    if (baseType->IsClass()) {
      PrintNativeType(result, baseType);
      result << "*";
    } else {
      result << "Object*";
    }
  } else if (type->IsArray()) {
    ArrayType* arrayType = static_cast<ArrayType*>(type);
    Type*      elementType = arrayType->GetElementType();
    if (elementType->IsVector()) {
      PrintNativeType(result, static_cast<VectorType*>(type)->GetElementType());
    } else if (elementType->IsMatrix()) {
      PrintNativeType(result, static_cast<MatrixType*>(type)->GetColumnType()->GetElementType());
    } else {
      PrintNativeType(result, elementType);
    }
    result << "*";
  } else if (type->IsVector()) {
    result << "const ";
    PrintNativeType(result, static_cast<VectorType*>(type)->GetElementType());
    result << "*";
  } else if (type->IsMatrix()) {
    PrintNativeType(result, static_cast<MatrixType*>(type)->GetColumnType()->GetElementType());
    result << "*";
  } else if (type->IsQualified()) {
    PrintNativeType(result, static_cast<QualifiedType*>(type)->GetBaseType());
  } else {
    std::cerr << "PrintNativeType():  unknown type \"" << type->ToString() << "\"\n";
    exit(-2);
  }
}

void GenBindings::EmitMethod(Method* method) {
  int classTypeID = EmitType(method->classType);
  int returnTypeID = EmitType(method->returnType);
  file_ << "  m = new Method(0";
  if (method->modifiers & Method::Modifier::Static) { file_ << " | Method::Modifier::Static"; }
  if (method->modifiers & Method::Modifier::DeviceOnly) { file_ << " | Method::Modifier::DeviceOnly"; }
  if (method->modifiers & Method::Modifier::Vertex) { file_ << " | Method::Modifier::Vertex"; }
  if (method->modifiers & Method::Modifier::Fragment) { file_ << " | Method::Modifier::Fragment"; }
  if (method->modifiers & Method::Modifier::Compute) { file_ << " | Method::Modifier::Compute"; }
  std::string name = method->name;
  file_ << ", type" << returnTypeID << ", \"" << name << "\", type" << classTypeID << ");\n";
  const VarVector& argList = method->formalArgList;
  for (int i = 0; i < argList.size(); ++i) {
    Var* var = argList[i].get();
    int defaultValueId = -1;
    if (emitSymbolsAndStatements_ && method->defaultArgs[i]) {
      defaultValueId = sourcePass_.Resolve(method->defaultArgs[i]);
    }
    int varTypeID = EmitType(var->type);
    file_ << "  m->AddFormalArg(\"" << var->name << "\", type" << varTypeID << ", ";
    if (defaultValueId >= 0) {
      file_ << "node" << defaultValueId;
    } else {
      file_ << "nullptr";
    }
    file_ << ");\n";
  }
  file_ << "  c->AddMethod(m);\n";
  bool skipFirst = false;
  if (method->IsNative()) {
    if (header_ && !(method->modifiers & Method::Modifier::DeviceOnly)) {
#if TARGET_OS_IS_WIN
      header_ << "__declspec(dllexport) ";
#endif
      PrintNativeType(header_, method->returnType);
      header_ << " " << method->GetMangledName() << "(";
      if (method->IsConstructor()) {
        skipFirst = true;
        if (method->classType->IsClassTemplate()) {
          header_ << "int qualifiers, ";
          ClassTemplate* classTemplate = static_cast<ClassTemplate*>(method->classType);
          for (Type* arg : classTemplate->GetFormalTemplateArgs()) {
            FormalTemplateArg* formalTemplateArg = static_cast<FormalTemplateArg*>(arg);
            header_ << "Type* " << formalTemplateArg->GetName();
            if (arg != classTemplate->GetFormalTemplateArgs().back() || !argList.empty()) {
              header_ << ", ";
            }
          }
        }
      }
      for (const std::shared_ptr<Var>& var : argList) {
        if (skipFirst) { skipFirst = false; continue; }
        PrintNativeType(header_, var->type);
        if (var->name == "this") {
          header_ << " This";
        } else {
          header_ << " " << var->name;
        }
        if (&var != &argList.back()) { header_ << ", "; }
      }
      header_ << ");\n";
    }
  }
  if (emitSymbolsAndStatements_) assert(!method->stmts);
  if (!method->spirv.empty()) {
    file_ << "  m->spirv = {\n";
    for (uint32_t op : method->spirv) {
      file_ << op << ", ";
    }
    file_ << "};\n";
  }
  if (!method->wgsl.empty()) { file_ << "  m->wgsl = R\"(" << method->wgsl << ")\";\n"; }
}

void GenBindings::EmitClass(ClassType* classType) {
  if (classType->GetTemplate() && classType->GetTemplate()->HasNativeMethods()) { return; }
  auto id = EmitType(classType);
  file_ << "\n  c = type" << id << ";\n";
  if (classType->HasNativeMethods() && !classType->GetTemplate()) {
    file_ << "  NativeClass::" << classType->GetName() << " = c;\n";
  }
  file_ << "  c->SetMemoryLayout(MemoryLayout::" <<
    MemoryLayoutToString(classType->GetMemoryLayout()) << ");\n";
  if (ClassType* parent = classType->GetParent()) {
    int parentID = EmitType(classType->GetParent());
    file_ << "  c->SetParent(type" << parentID << ");\n";
  }
  for (const auto& field : classType->GetFields()) {
    int typeID = EmitType(field->type);
    int defaultValueId = -1;
    if (emitSymbolsAndStatements_ && field->defaultValue) {
      defaultValueId = sourcePass_.Resolve(field->defaultValue);
    }
    file_ << "  c->AddField(\"" << field->name << "\", type" << EmitType(field->type) << ", ";
    if (defaultValueId >= 0) {
      file_ << "node" << defaultValueId;
    } else {
      file_ << "nullptr";
    }
    file_ << ");\n";
  }
  if (emitSymbolsAndStatements_) {
    for (const auto& constant : classType->GetConstants()) {
      int valueId = sourcePass_.Resolve(constant.second);
      file_ << "  c->AddConstant(\"" << constant.first << "\", " << "node" << valueId << ");\n";
    }
  }
  for (const auto& method : classType->GetMethods()) {
    EmitMethod(method.get());
  }
  if (emitSymbolsAndStatements_) {
    for (const auto& pair : classType->GetTypes()) {
      int typeID = EmitType(pair.second);
      file_ << "  c->DefineType(\"" << pair.first << "\", ";
      if (typeID >= 0) {
        file_ << "type" << typeID;
      } else {
        file_ << "nullptr";
      }
      file_ << ");\n";
    }
  }
  if (emitSymbolsAndStatements_ && !classType->GetTemplate()) {
    file_ << "  rootStmts->DefineType(\"" << classType->GetName() << "\", c);\n";
  }
}

};  // namespace Toucan
