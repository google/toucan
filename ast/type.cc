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

#include "type.h"

#include <assert.h>
#include <string.h>

#include <algorithm>

#include "ast.h"
#include "symbol.h"

namespace Toucan {

namespace {

inline int roundUpTo(int modulus, int value) { return (value + modulus - 1) / modulus * modulus; }

std::string QualifiersToString(int qualifiers, std::string sep) {
  std::string result;
  if (qualifiers & Type::Qualifier::Uniform) { result += "uniform" + sep; }
  if (qualifiers & Type::Qualifier::Storage) { result += "storage" + sep; }
  if (qualifiers & Type::Qualifier::Vertex) { result += "vertex" + sep; }
  if (qualifiers & Type::Qualifier::Index) { result += "index" + sep; }
  if (qualifiers & Type::Qualifier::Sampleable) { result += "sampleable" + sep; }
  if (qualifiers & Type::Qualifier::Renderable) { result += "renderable" + sep; }
  if (qualifiers & Type::Qualifier::ReadOnly) { result += "readonly" + sep; }
  if (qualifiers & Type::Qualifier::WriteOnly) { result += "writeonly" + sep; }
  if (qualifiers & Type::Qualifier::Coherent) { result += "coherent" + sep; }
  return result;
}

}  // namespace

BoolType::BoolType() {}

IntegerType::IntegerType(int bits, bool s) : bits_(bits), signed_(s) {}

bool IntegerType::CanWidenTo(Type* other) const {
  if (!other->IsInteger()) { return false; }
  IntegerType* otherInteger = static_cast<IntegerType*>(other);
  return (otherInteger->GetBits() >= bits_);
}

std::string IntegerType::ToString() const {
  switch (bits_) {
    case 32: return signed_ ? "int" : "uint";
    case 16: return signed_ ? "short" : "ushort";
    case 8: return signed_ ? "byte" : "ubyte";
  }
  assert(!"unsupported integer bit width");
  return "";
}

FloatingPointType::FloatingPointType(int bits) : bits_(bits) {}

bool FloatingPointType::CanWidenTo(Type* other) const {
  if (!other->IsFloatingPoint()) { return false; }
  FloatingPointType* otherFloatingPoint = static_cast<FloatingPointType*>(other);
  return (otherFloatingPoint->GetBits() >= bits_);
}

std::string FloatingPointType::ToString() const {
  switch (bits_) {
    case 32: return "float";
    case 64: return "double";
  }
  assert(!"unsupported floating point bit width");
  return "";
}

StringType::StringType() {}

VoidType::VoidType() {}

AutoType::AutoType() {}

NullType::NullType() : PtrType(nullptr) {}

ListType::ListType(const VarVector& types) : types_(types) {}

int ListType::GetSizeInBytes() const {
  int size = 0;
  for (auto type : types_) {
    size += type->type->GetSizeInBytes();
  }
  return size;
}

FormalTemplateArg::FormalTemplateArg(std::string name) : name_(name) {}

std::string FormalTemplateArg::ToString() const { return name_; }

VectorType::VectorType(Type* componentType, unsigned int length)
    : componentType_(componentType), length_(length) {}

int VectorType::GetSizeInBytes() const {
  unsigned int length = length_ == 3 ? 4 : length_;
  return length * componentType_->GetSizeInBytes();
}

std::string VectorType::ToString() const {
  return componentType_->ToString() + "<" + std::to_string(length_) + ">";
}

bool VectorType::CanWidenTo(Type* type) const {
  if (type == this) return true;
  if (type->IsVector()) {
    VectorType* vectorType = static_cast<VectorType*>(type);
    return length_ == vectorType->GetLength() &&
           componentType_->CanWidenTo(vectorType->GetComponentType());
  }
  return false;
}

bool VectorType::CanInitFrom(const ListType* listType) const {
  if (listType->IsNamed()) { return false; }

  auto types = listType->GetTypes();
  if (types.size() != length_) { return false; }
  for (auto type : types) {
    if (!type->type->CanWidenTo(componentType_)) { return false; }
  }
  return true;
}

int VectorType::GetSwizzle(const std::string& str) const {
  if (str[0] == '\0') return -1;
  if (str[1] != '\0') return -1;
  char c = str[0];
  if (c == 'x' || c == 'r') return 0;
  if (c == 'y' || c == 'g') return 1;
  if ((c == 'z' || c == 'b') && length_ >= 3) return 2;
  if ((c == 'w' || c == 'a') && length_ >= 4) return 3;
  return -1;
}

MatrixType::MatrixType(VectorType* columnType, unsigned int numColumns)
    : columnType_(columnType), numColumns_(numColumns) {}

std::string MatrixType::ToString() const {
  return columnType_->ToString() + "<" + std::to_string(numColumns_) + ">";
}

bool MatrixType::CanInitFrom(const ListType* listType) const {
  if (listType->IsNamed()) { return false; }
  auto types = listType->GetTypes();
  if (types.size() != numColumns_) { return false; }
  for (auto type : types) {
    if (!type->type->CanWidenTo(columnType_)) { return false; }
  }
  return true;
}

ArrayType::ArrayType(Type* elementType, int numElements, MemoryLayout memoryLayout)
    : elementType_(elementType), numElements_(numElements), memoryLayout_(memoryLayout) {}

bool ArrayType::CanWidenTo(Type* type) const {
  if (type->IsArray()) {
    auto arrayType = static_cast<ArrayType*>(type);
    if (arrayType->GetElementType() != GetElementType()) { return false; }
    if (arrayType->GetNumElements() == 0) { return true; }
    if (arrayType->GetNumElements() == GetNumElements()) { return true; }
  }
  return false;
}

bool ArrayType::CanInitFrom(const ListType* listType) const {
  if (listType->IsNamed()) { return false; }
  auto types = listType->GetTypes();
  if (types.empty()) { return true; }
  for (auto type : types) {
    if (!type->type->CanWidenTo(elementType_)) { return false; }
  }
  return true;
}

int ArrayType::GetElementSizeInBytes() const {
  int elementSize = GetElementType()->GetSizeInBytes();
  elementSize = roundUpTo(elementType_->GetAlignmentInBytes(), elementSize);
  if (memoryLayout_ == MemoryLayout::Uniform) { elementSize = roundUpTo(16, elementSize); }
  return elementSize;
}

int ArrayType::GetElementPadding() const {
  return GetElementSizeInBytes() - elementType_->GetSizeInBytes();
}

int ArrayType::GetSizeInBytes() const { return GetElementSizeInBytes() * numElements_; }

int ArrayType::GetAlignmentInBytes() const { return elementType_->GetAlignmentInBytes(); }

int ArrayType::GetSizeInBytes(int dynamicArrayLength) const {
  int elementSize = GetElementSizeInBytes();
  if (numElements_ > 0) {
    return elementSize * numElements_;
  } else {
    return elementSize * dynamicArrayLength;
  }
}

std::string ArrayType::ToString() const {
  std::string result = "[";
  if (numElements_ > 0) { result += std::to_string(numElements_); }
  result += "]" + elementType_->ToString();
  return result;
}

QualifiedType::QualifiedType(Type* base, int qualifiers)
    : baseType_(base), qualifiers_(qualifiers) {}

bool QualifiedType::CanWidenTo(Type* type) const {
  if (type == this) return true;
  if (type->IsQualified()) {
    auto* qualifiedType = static_cast<QualifiedType*>(type);
    if (!baseType_->CanWidenTo(qualifiedType->GetBaseType())) { return false; }
    int qualifiers = qualifiedType->GetQualifiers();
    return (qualifiers_ & qualifiers) == qualifiers;
  } else if (baseType_->CanWidenTo(type)) {
    return true;
  }
  return false;
}

Type* QualifiedType::GetUnqualifiedType(int* qualifiers) {
  if (qualifiers) { *qualifiers = qualifiers_; }
  return baseType_;
}

std::string QualifiedType::ToString() const {
  return QualifiersToString(qualifiers_, " ") + baseType_->ToString();
}

UnresolvedScopedType::UnresolvedScopedType(FormalTemplateArg* baseType, std::string id)
    : baseType_(baseType), id_(id) {}

std::string UnresolvedScopedType::ToString() const { return baseType_->ToString() + "::" + id_; }

EnumType::EnumType(std::string name) : name_(name), nextValue_(0) {}

void EnumType::Append(std::string id) { values_.push_back(EnumValue(this, id, nextValue_++)); }

void EnumType::Append(std::string id, int value) {
  values_.push_back(EnumValue(this, id, value));
  nextValue_ = value + 1;
}

int EnumType::GetSizeInBytes() const { return 4; }

std::string EnumType::ToString() const { return name_; }

bool EnumType::CanWidenTo(Type* type) const {
  if (type == this) return true;
  return type->IsInt() || type->IsUInt();
}

Method::Method(int m, Type* r, std::string n, ClassType* c)
    : modifiers(m), returnType(r), name(n), classType(c) {}

std::string Method::ToString() const {
  std::string result;
  bool isStatic = (modifiers & Method::Modifier::Static) != 0;
  result += classType->ToString() + "." + name + "(";
  for (const auto& arg : formalArgList) {
    if (arg == formalArgList.front() && !isStatic) { continue; }
    result += arg->type->ToString();
    if (arg != formalArgList.back()) { result += ", "; }
  }
  result += ") ";
  if (!isStatic) {
    int qualifiers;
    Type* thisType = formalArgList.front()->type;
    assert(thisType->IsPtr());
    thisType = static_cast<PtrType*>(thisType)->GetBaseType();
    thisType->GetUnqualifiedType(&qualifiers);
    if (qualifiers) {
      result += QualifiersToString(qualifiers, " ");
    }
  }
  if (!returnType->IsVoid()) { result += ": " + returnType->ToString(); }
  return result;
}

std::string Method::GetMangledName() const {
  std::string result = classType->GetName() + "_";
  if (name[0] == '~') {
    result += "Destroy";
  } else {
    result += name;
  }
  for (auto& m : classType->GetMethods()) {
    if (m.get() != this && m->name == name) {
      for (auto arg : formalArgList) {
        result += "_";
        if (arg->type->IsPtr()) {
          auto baseType = static_cast<PtrType*>(arg->type)->GetBaseType();
          int qualifiers;
          baseType = baseType->GetUnqualifiedType(&qualifiers);
          result += QualifiersToString(qualifiers, "_");
          if (baseType->IsClass()) {
            result += static_cast<ClassType*>(baseType)->GetName();
          } else {
            result += baseType->ToString();
          }
        } else if (arg->type->IsClass()) {
          result += static_cast<ClassType*>(arg->type)->GetName();
        } else if (arg->type->IsVector()) {
          auto vectorType = static_cast<VectorType*>(arg->type);
          result +=
              vectorType->GetComponentType()->ToString() + std::to_string(vectorType->GetLength());
        } else {
          result += arg->type->ToString();
        }
      }
      break;
    }
  }
  return result;
}

ClassType::ClassType(std::string name)
    : name_(name), parent_(nullptr), template_(nullptr), numFields_(0) {
  vtable_.resize(1);
}

bool ClassType::IsPOD() const {
  for (const auto& it : fields_) {
    if (!it->type->IsPOD()) { return false; }
  }
  return true;
}

bool ClassType::IsFullySpecified() const {
  if (!template_) { return true; }

  if (templateArgs_.size() < template_->GetFormalTemplateArgs().size()) { return false; }

  for (auto* arg : templateArgs_) {
    if (!arg->IsFullySpecified()) { return false; }
  }

  return true;
}

void ClassType::SetParent(ClassType* parent) {
  assert(parent_ == nullptr);
  if (parent == nullptr) { return; }
  numFields_ += parent->numFields_;
  vtable_ = parent->vtable_;
  parent_ = parent;
}

Field* ClassType::AddField(std::string name, Type* type, Expr* defaultValue) {
  fields_.push_back(std::make_unique<Field>(name, type, numFields_, this, defaultValue));
  numFields_++;
  return fields_.back().get();
}

void ClassType::SetMemoryLayout(MemoryLayout memoryLayout, TypeTable* types) {
  if (memoryLayout_ >= memoryLayout) { return; }
  memoryLayout_ = memoryLayout;
  for (const auto& field : GetFields()) {
    if (field->type->IsClass()) {
      static_cast<ClassType*>(field->type)->SetMemoryLayout(memoryLayout, types);
    } else if (field->type->IsArray()) {
      auto arrayType = static_cast<ArrayType*>(field->type);
      field->type = types->GetArrayType(arrayType->GetElementType(), arrayType->GetNumElements(),
                                        memoryLayout);
    }
  }
}

size_t ClassType::ComputeFieldOffsets() {
  size_t offset = 0;
  if (parent_) { offset = parent_->ComputeFieldOffsets(); }
  Field* prevField = nullptr;
  for (const auto& field : GetFields()) {
    size_t size = field->type->GetSizeInBytes();
    size_t alignment = field->type->GetAlignmentInBytes();
    if (size_t padding = offset % alignment) {
      offset += alignment - padding;
      prevField->padding = padding;
    }
    field->offset = offset;
    offset += size;
    prevField = field.get();
  }
  padding_ = GetSizeInBytes(0) - offset;
  return offset;
}

void ClassType::AddMethod(Method* method) {
  methods_.push_back(std::unique_ptr<Method>(method));
}

void ClassType::AddEnum(std::string name, EnumType* enumType) {
  enums_.push_back(std::unique_ptr<EnumType>(enumType));
}

Type* ClassType::FindType(const std::string& id) {
  if (Type* type = scope_->types[id]) { return type; }
  return parent_ ? parent_->FindType(id) : nullptr;
}

Field* ClassType::FindField(std::string name) const {
  for (const auto& it : fields_) {
    Field* field = it.get();
    if (field->name == name) { return field; }
  }
  return parent_ ? parent_->FindField(name) : nullptr;
}

int ClassType::GetSizeInBytes() const {
  assert(!HasUnsizedArray());
  return GetSizeInBytes(0);
}

int ClassType::GetAlignmentInBytes() const {
  int result = parent_ ? parent_->GetAlignmentInBytes() : 1;
  for (const auto& it : fields_) {
    result = std::max(result, it->type->GetAlignmentInBytes());
  }
  if (memoryLayout_ == MemoryLayout::Uniform) { result = roundUpTo(16, result); }
  return result;
}

int ClassType::GetSizeInBytes(int dynamicArrayLength) const {
  int size = parent_ ? parent_->GetSizeInBytes() : 0;
  for (const auto& it : fields_) {
    size += it->type->GetSizeInBytes(dynamicArrayLength);
    size = roundUpTo(it->type->GetAlignmentInBytes(), size);
  }
  int alignment = GetAlignmentInBytes();
  size = roundUpTo(alignment, size);
  return size;
}

bool ClassType::CanWidenTo(Type* type) const {
  for (const ClassType* t = this; t != nullptr; t = t->GetParent()) {
    if (t == type) { return true; }
  }
  return false;
}

bool ClassType::CanInitFrom(const ListType* listType) const {
  auto types = listType->GetTypes();
  if (types.empty()) { return true; }
  if (listType->IsNamed()) {
    for (auto type : types) {
      Field* field = FindField(type->name);
      if (!field) { return false; }
      if (!type->type->CanWidenTo(field->type)) { return false; }
    }
  } else {
    if (types.size() != fields_.size()) { return false; }
  }
  return true;
}

bool ClassType::IsNative() const {
  if (template_) { return template_->IsNative(); }
  return native_;
}

bool ClassType::HasUnsizedArray() const {
  if (fields_.size() == 0) { return false; }
  return (fields_.back()->type->IsUnsizedArray());
}

void ClassType::SetVTable(int index, Method* method) {
  assert(index >= 0 && index < vtable_.size());
  method->index = index;
  vtable_[index] = method;
}

void ClassType::AppendToVTable(Method* method) {
  method->index = vtable_.size();
  vtable_.push_back(method);
}

void Method::AddFormalArg(std::string name, Type* type, Expr* defaultValue) {
  formalArgList.push_back(std::make_shared<Var>(name, type));
  defaultArgs.push_back(defaultValue);
}

std::string ClassType::ToString() const {
  std::string result = name_;
  if (templateArgs_.size() > 0) {
    result += "<";
    for (size_t i = 0; i < templateArgs_.size(); ++i) {
      if (i > 0) { result += ", "; }
      result += templateArgs_[i]->ToString();
    }
    result += ">";
  }
  return result;
}

ClassTemplate::ClassTemplate(std::string name, const TypeList& formalTemplateArgs)
    : ClassType(name), formalTemplateArgs_(formalTemplateArgs) {}

PtrType::PtrType(Type* baseType) : baseType_(baseType) {}

StrongPtrType::StrongPtrType(Type* baseType) : PtrType(baseType) {}

WeakPtrType::WeakPtrType(Type* baseType) : PtrType(baseType) {}

bool WeakPtrType::CanWidenTo(Type* type) const {
  if (type == this) return true;
  if (type->IsWeakPtr()) {
    WeakPtrType* weakPtrType = static_cast<WeakPtrType*>(type);
    if (!GetBaseType() || GetBaseType()->CanWidenTo(weakPtrType->GetBaseType())) { return true; }
  }
  return false;
};
RawPtrType::RawPtrType(Type* baseType) : PtrType(baseType) {}

std::string StrongPtrType::ToString() const { return "*" + GetBaseType()->ToString(); };

bool StrongPtrType::CanWidenTo(Type* type) const {
  if (type == this) return true;
  if (type->IsPtr()) {
    PtrType* ptrType = static_cast<PtrType*>(type);
    if (ptrType->GetBaseType()->IsVoid()) { return true; }
    if (GetBaseType()->CanWidenTo(ptrType->GetBaseType())) { return true; }
  }
  return false;
};

std::string WeakPtrType::ToString() const { return "^" + GetBaseType()->ToString(); };

std::string RawPtrType::ToString() const { return "&" + GetBaseType()->ToString(); };

TypeTable::TypeTable() {
  bool_ = Make<BoolType>();
  string_ = Make<StringType>();
  void_ = Make<VoidType>();
  auto_ = Make<AutoType>();
  null_ = Make<NullType>();
}

IntegerType* TypeTable::GetInteger(int bits, bool isSigned) {
  int          key = isSigned ? -bits : bits;
  IntegerType* type = integerTypes_[key];
  if (type == nullptr) {
    type = Make<IntegerType>(bits, isSigned);
    integerTypes_[key] = type;
  }
  return type;
}

FloatingPointType* TypeTable::GetFloatingPoint(int bits) {
  FloatingPointType* type = floatingPointTypes_[bits];
  if (type == nullptr) {
    type = Make<FloatingPointType>(bits);
    floatingPointTypes_[bits] = type;
  }
  return type;
}

IntegerType* TypeTable::GetByte() { return GetInteger(8, true); }

IntegerType* TypeTable::GetUByte() { return GetInteger(8, false); }

IntegerType* TypeTable::GetShort() { return GetInteger(16, true); }

IntegerType* TypeTable::GetUShort() { return GetInteger(16, false); }

IntegerType* TypeTable::GetInt() { return GetInteger(32, true); }

IntegerType* TypeTable::GetUInt() { return GetInteger(32, false); }

FloatingPointType* TypeTable::GetFloat() { return GetFloatingPoint(32); }

FloatingPointType* TypeTable::GetDouble() { return GetFloatingPoint(64); }

BoolType* TypeTable::GetBool() { return bool_; }

VectorType* TypeTable::GetVector(Type* componentType, int size) {
  if (size < 2 || size > 4) return nullptr;
  VectorType* type = vectorTypes_[TypeAndInt(componentType, size)];
  if (type == nullptr) {
    type = Make<VectorType>(componentType, size);
    vectorTypes_[TypeAndInt(componentType, size)] = type;
  }
  return type;
}

MatrixType* TypeTable::GetMatrix(VectorType* columnType, int numColumns) {
  if (numColumns < 2 || numColumns > 4) return nullptr;
  MatrixType* type = matrixTypes_[TypeAndInt(columnType, numColumns)];
  if (type == nullptr) {
    type = Make<MatrixType>(columnType, numColumns);
    matrixTypes_[TypeAndInt(columnType, numColumns)] = type;
  }
  return type;
}

StringType* TypeTable::GetString() { return string_; }

VoidType* TypeTable::GetVoid() { return void_; }

AutoType* TypeTable::GetAuto() { return auto_; }

NullType* TypeTable::GetNull() { return null_; }

ListType* TypeTable::GetList(VarVector&& types) {
  // FIXME: implement caching?
  return Make<ListType>(types);
}

StrongPtrType* TypeTable::GetStrongPtrType(Type* baseType) {
  StrongPtrType* type = strongPtrTypes_[baseType];
  if (type == nullptr) {
    type = Make<StrongPtrType>(baseType);
    strongPtrTypes_[baseType] = type;
  }
  return type;
}

WeakPtrType* TypeTable::GetWeakPtrType(Type* baseType) {
  WeakPtrType* type = weakPtrTypes_[baseType];
  if (type == nullptr) {
    type = Make<WeakPtrType>(baseType);
    weakPtrTypes_[baseType] = type;
  }
  return type;
}

RawPtrType* TypeTable::GetRawPtrType(Type* baseType) {
  RawPtrType* type = rawPtrTypes_[baseType];
  if (type == nullptr) {
    type = Make<RawPtrType>(baseType);
    rawPtrTypes_[baseType] = type;
  }
  return type;
}

TypeList* TypeTable::AppendTypeList(TypeList* typeList) {
  typeLists_.push_back(std::unique_ptr<TypeList>(typeList));
  return typeList;
}

ArrayType* TypeTable::GetArrayType(Type* elementType, int size, MemoryLayout memoryLayout) {
  ArrayTypeKey key(TypeAndInt(elementType, size), memoryLayout);
  ArrayType*   type = arrayTypes_[key];
  if (type == nullptr) {
    type = Make<ArrayType>(elementType, size, memoryLayout);
    arrayTypes_[key] = type;
  }
  return type;
}

FormalTemplateArg* TypeTable::GetFormalTemplateArg(std::string name) {
  FormalTemplateArg* type = formalTemplateArgs_[name];
  if (type == nullptr) {
    type = Make<FormalTemplateArg>(name);
    formalTemplateArgs_[std::string(name)] = type;
  }
  return type;
}

Type* TypeTable::GetQualifiedType(Type* type, int qualifiers) {
  if (qualifiers == 0) { return type; }
  int currentQualifiers;
  type = type->GetUnqualifiedType(&currentQualifiers);
  qualifiers |= currentQualifiers;
  TypeAndInt key(type, qualifiers);
  if (auto result = qualifiedTypes_[key]) { return result; }
  QualifiedType* result = Make<QualifiedType>(type, qualifiers);
  qualifiedTypes_[key] = result;
  return result;
}

Type* TypeTable::GetUnresolvedScopedType(FormalTemplateArg* baseType, std::string id) {
  TypeAndId key(baseType, id);
  if (Type* result = unresolvedScopedTypes_[key]) { return result; }

  auto* result = Make<UnresolvedScopedType>(baseType, id);
  unresolvedScopedTypes_[key] = result;
  return result;
}

ClassType* TypeTable::GetClassTemplateInstance(ClassTemplate*  classTemplate,
                                               const TypeList& templateArgs) {
  for (ClassType* const& i : classTemplate->GetInstances()) {
    if (i->GetTemplateArgs() == templateArgs) { return i; }
  }
  std::string name = classTemplate->GetName();
  ClassType*  instance = Make<ClassType>(name);
  instance->SetTemplate(classTemplate);
  instance->SetTemplateArgs(templateArgs);
  classTemplate->AddInstance(instance);
  if (instance->IsFullySpecified()) { instanceQueue_.push_back(instance); }
  return instance;
}

ClassType* TypeTable::PopInstanceQueue() {
  if (instanceQueue_.empty()) { return nullptr; }
  ClassType* instance = instanceQueue_.back();
  instanceQueue_.pop_back();
  return instance;
}

int TypeTable::GetTypeID(Type* type) const {
  for (int i = 0; i < types_.size(); i++) {
    if (types_[i] == type) { return i; }
  }
  return -1;
}

bool TypeTable::VectorScalar(Type* lhs, Type* rhs) {
  return lhs->IsVector() && static_cast<VectorType*>(lhs)->GetComponentType() == rhs;
}

bool TypeTable::ScalarVector(Type* lhs, Type* rhs) { return VectorScalar(rhs, lhs); }

bool TypeTable::MatrixScalar(Type* lhs, Type* rhs) {
  return lhs->IsMatrix() &&
         static_cast<MatrixType*>(lhs)->GetColumnType()->GetComponentType() == rhs;
}

bool TypeTable::ScalarMatrix(Type* lhs, Type* rhs) { return MatrixScalar(rhs, lhs); }

bool TypeTable::MatrixVector(Type* lhs, Type* rhs) {
  return lhs->IsMatrix() && static_cast<MatrixType*>(lhs)->GetColumnType() == rhs;
}

bool TypeTable::VectorMatrix(Type* lhs, Type* rhs) { return MatrixVector(rhs, lhs); }

void TypeTable::Layout() {
  std::vector<Type*> classTypes;
  for (auto type : types_) {
    if (type->GetUnqualifiedType()->IsClass()) { classTypes.push_back(type); }
  }
  for (Type* type : classTypes) {
    int qualifiers;
    type = type->GetUnqualifiedType(&qualifiers);
    if (type->IsClass()) {
      auto classType = static_cast<ClassType*>(type);
      if (qualifiers & Type::Qualifier::Uniform) {
        classType->SetMemoryLayout(MemoryLayout::Uniform, this);
      } else if (qualifiers & Type::Qualifier::Storage) {
        classType->SetMemoryLayout(MemoryLayout::Storage, this);
      }
    }
  }
  for (Type* type : classTypes) {
    if (type->IsClass() && !type->IsClassTemplate()) {
      static_cast<ClassType*>(type)->ComputeFieldOffsets();
    }
  }
}

};  // namespace Toucan
