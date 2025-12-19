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

#ifndef _AST_TYPE_H
#define _AST_TYPE_H

#include <assert.h>
#include <stdio.h>

#include <array>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include <utils/hash_pair.h>

namespace Toucan {

class Expr;
class TypeTable;
class ClassType;
class ListType;

enum class MemoryLayout { Default = 0, Storage = 1, Uniform = 2 };

class Type {
 public:
  virtual ~Type(){};
  virtual bool  IsArray() const { return false; }
  virtual bool  IsArrayLike() const { return false; }
  virtual bool  IsQualified() const { return false; }
  virtual bool  IsUnsizedArray() const { return false; }
  virtual bool  IsUnsizedClass() const { return false; }
  virtual bool  IsVector() const { return false; }
  virtual bool  IsByte() const { return false; }
  virtual bool  IsUByte() const { return false; }
  virtual bool  IsShort() const { return false; }
  virtual bool  IsUShort() const { return false; }
  virtual bool  IsUnsigned() const { return false; }
  virtual bool  IsInteger() const { return false; }
  virtual bool  IsIntegerVector() const { return false; }
  virtual bool  IsFloatingPoint() const { return false; }
  virtual bool  IsFloatVector() const { return false; }
  virtual bool  IsMatrix() const { return false; }
  virtual bool  IsBool() const { return false; }
  virtual bool  IsBoolVector() const { return false; }
  virtual bool  IsInt() const { return false; }
  virtual bool  IsUInt() const { return false; }
  virtual bool  IsFloat() const { return false; }
  virtual bool  IsDouble() const { return false; }
  virtual bool  IsEnum() const { return false; }
  virtual bool  IsList() const { return false; }
  virtual bool  IsStrongPtr() const { return false; }
  virtual bool  IsWeakPtr() const { return false; }
  virtual bool  IsRawPtr() const { return false; }
  virtual bool  IsPtr() const { return false; }
  virtual bool  IsString() const { return false; }
  virtual bool  IsVoid() const { return false; }
  virtual bool  IsAuto() const { return false; }
  virtual bool  IsClass() const { return false; }
  virtual bool  IsClassTemplate() const { return false; }
  virtual bool  IsFormalTemplateArg() const { return false; }
  virtual bool  IsUnresolvedScopedType() const { return false; }
  virtual bool  IsFullySpecified() const { return true; }
  virtual bool  IsReadable() const { return true; }
  virtual bool  IsWriteable() const { return true; }
  virtual bool  NeedsDestruction() const { return false; }
  virtual Type* GetUnqualifiedType(int* qualifiers = nullptr) {
    if (qualifiers) *qualifiers = 0;
    return this;
  }
  Type*               CheckAndRemoveQualifiers(Type* type) const;
  virtual std::string ToString() const = 0;
  virtual int         GetSizeInBytes() const = 0;
  virtual int         GetSizeInBytes(int dynamicArrayLength) const { return GetSizeInBytes(); }
  virtual int         GetAlignmentInBytes() const { return GetSizeInBytes(); }
  virtual bool        CanWidenTo(Type* type) const;
  virtual bool        CanNarrowTo(Type* type) const { return type == this; }
  virtual bool        CanInitFrom(const ListType* type) const { return false; }
  virtual bool        ContainsRawPtr() const { return false; }
  enum Qualifier {
    Uniform = 0x0001,
    Storage = 0x0002,
    Vertex = 0x0004,
    Index = 0x0008,
    Sampleable = 0x0010,
    Renderable = 0x0020,
    ReadOnly = 0x0040,
    WriteOnly = 0x0080,
    HostReadable = 0x100,
    HostWriteable = 0x200,
    Unfilterable = 0x0400,
    Coherent = 0x0800,
  };
};

using TypeList = std::vector<Type*>;
using TypeMap = std::unordered_map<std::string, Type*>;

class ArrayLikeType : public Type {
 public:
  ArrayLikeType(Type* elementType, uint32_t size);
  bool              IsArrayLike() const override { return true; }
  Type*             GetElementType() const { return elementType_; }
  uint32_t          GetNumElements() const { return numElements_; }
 protected:
  Type*             elementType_;
  uint32_t          numElements_;
};

class VectorType : public ArrayLikeType {
 public:
  VectorType(Type* componentType, uint32_t size);
  bool         IsVector() const override { return true; }
  bool         IsUnsigned() const override { return elementType_->IsUnsigned(); }
  bool         IsIntegerVector() const override { return elementType_->IsInteger(); }
  bool         IsFloatVector() const override { return elementType_->IsFloat(); }
  bool         IsBoolVector() const override { return elementType_->IsBool(); }
  bool         CanWidenTo(Type* type) const override;
  bool         CanNarrowTo(Type* type) const override;
  bool         CanInitFrom(const ListType* type) const override;
  int          GetSizeInBytes() const override;
  std::string  ToString() const override;
  int          GetSwizzle(const std::string& str) const;
};

class MatrixType : public ArrayLikeType {
 public:
  MatrixType(VectorType* columnType, uint32_t numColumns);
  bool IsMatrix() const override { return true; }
  int  GetSizeInBytes() const override { return numElements_ * elementType_->GetSizeInBytes(); }
  std::string  ToString() const override;
  VectorType*  GetColumnType() { return static_cast<VectorType*>(elementType_); }
  unsigned int GetNumColumns() { return numElements_; }
  bool         CanInitFrom(const ListType* type) const override;
};

class IntegerType : public Type {
 public:
  explicit IntegerType(int bits, bool isSigned);
  bool        IsInteger() const override { return true; }
  std::string ToString() const override;
  int         GetSizeInBytes() const override { return bits_ / 8; }
  int         GetBits() const { return bits_; }
  bool        Signed() const { return signed_; }
  bool        IsUnsigned() const override { return !signed_; }
  bool        CanWidenTo(Type* type) const override;
  bool        CanNarrowTo(Type* type) const override;

  bool IsByte() const override { return bits_ == 8 && signed_ == true; }  // FIXME remove these?
  bool IsShort() const override { return bits_ == 16 && signed_ == true; }
  bool IsInt() const override { return bits_ == 32 && signed_ == true; }

  bool IsUByte() const override { return bits_ == 8 && signed_ == false; }
  bool IsUShort() const override { return bits_ == 16 && signed_ == false; }
  bool IsUInt() const override { return bits_ == 32 && signed_ == false; }

 private:
  int  bits_;
  bool signed_;
};

class FloatingPointType : public Type {
 public:
  explicit FloatingPointType(int bits);
  bool        IsFloatingPoint() const override { return true; }
  std::string ToString() const override;
  int         GetSizeInBytes() const override { return bits_ / 8; }
  int         GetBits() const { return bits_; }
  bool        CanWidenTo(Type* type) const override;
  bool        CanNarrowTo(Type* type) const override;

  bool IsFloat() const override { return bits_ == 32; }
  bool IsDouble() const override { return bits_ == 64; }

 private:
  int bits_;
};

class BoolType : public Type {
 public:
  BoolType();
  bool        IsBool() const override { return true; }
  std::string ToString() const override { return "bool"; }
  int         GetSizeInBytes() const override { return 1; }
};

class EnumType;

struct EnumValue {
  EnumValue(EnumType* t, std::string i, int v) : type(t), id(i), value(v) {}
  EnumType*   type;
  std::string id;
  int         value;
};

typedef std::vector<EnumValue> EnumValueVector;

class VoidType : public Type {
 public:
  VoidType();
  bool        IsVoid() const override { return true; }
  std::string ToString() const override { return "void"; }
  int         GetSizeInBytes() const override { return 0; }
};

class AutoType : public Type {
 public:
  AutoType();
  bool        IsAuto() const override { return true; }
  std::string ToString() const override { return "auto"; }
  int         GetSizeInBytes() const override { return 0; }
};

class ArrayType : public ArrayLikeType {
 public:
  ArrayType(Type* elementType, uint32_t numElements, MemoryLayout memoryLayout);
  bool         IsArray() const override { return true; }
  bool         IsUnsizedArray() const override { return numElements_ == 0; }
  bool         IsFullySpecified() const override { return elementType_->IsFullySpecified(); }
  std::string  ToString() const override;
  int          GetElementSizeInBytes() const;
  int          GetElementPadding() const;
  int          GetAlignmentInBytes() const override;
  int          GetSizeInBytes() const override;
  int          GetSizeInBytes(int dynamicArrayLength) const override;
  MemoryLayout GetMemoryLayout() const { return memoryLayout_; }
  void         SetMemoryLayout(MemoryLayout memoryLayout) { memoryLayout_ = memoryLayout; }
  bool         CanWidenTo(Type* type) const override;
  bool         CanInitFrom(const ListType* type) const override;
  bool         NeedsDestruction() const override { return elementType_->NeedsDestruction(); }
  bool         ContainsRawPtr() const override { return elementType_->ContainsRawPtr(); }

 private:
  MemoryLayout memoryLayout_;
};

class QualifiedType : public Type {
 public:
  QualifiedType(Type* baseType, int qualifiers);
  bool          IsQualified() const override { return true; }
  bool          IsReadable() const override { return !(qualifiers_ & Type::Qualifier::WriteOnly); }
  bool          IsWriteable() const override { return !(qualifiers_ & Type::Qualifier::ReadOnly); }
  virtual Type* GetUnqualifiedType(int* qualifiers) override;
  Type*         GetBaseType() const { return baseType_; }
  int           GetQualifiers() const { return qualifiers_; }
  std::string   ToString() const override;
  int           GetSizeInBytes() const override { return baseType_->GetSizeInBytes(); }
  bool          NeedsDestruction() const override { return baseType_->NeedsDestruction(); }
  bool          CanWidenTo(Type* type) const override;
  bool          ContainsRawPtr() const override { return baseType_->ContainsRawPtr(); }

 private:
  Type* baseType_;
  int   qualifiers_;
};

struct Field {
  Field(std::string n, Type* t, int i, ClassType* c, Expr* d)
      : name(n), type(t), index(i), classType(c), defaultValue(d) {}
  std::string name;
  Type*       type;
  int         index;
  ClassType*  classType;
  Expr*       defaultValue;
  size_t      offset = 0;
  size_t      padding = 0;
};

typedef std::vector<std::unique_ptr<Field>> FieldVector;

struct Var {
  Var(const std::string& n, Type* t) : name(n), type(t) {}

  std::string name;
  Type*       type;
};

using VarVector = std::vector<std::shared_ptr<Var>>;

class Stmts;

struct Method {
  Method(int modifiers, Type* returnType, std::string name, ClassType* classType);
  std::string             ToString() const;
  std::string             GetMangledName() const;
  void                    AddFormalArg(std::string id, Type* type, Expr* defaultValue);
  bool                    IsNative() const { return !stmts; }
  bool                    IsConstructor() const;
  bool                    IsDestructor() const;
  int                     modifiers;
  Type*                   returnType;
  std::string             name;
  ClassType*              classType;
  Method*                 templateMethod = nullptr;
  std::array<uint32_t, 3> workgroupSize = {1, 1, 1};
  VarVector               formalArgList;
  std::vector<Expr*>      defaultArgs;
  Stmts*                  stmts = nullptr;
  Expr*                   initializer = nullptr;   // only used for constructors
  std::vector<uint32_t>   spirv;
  std::string             wgsl;
  int                     index = -1;
  enum Modifier {
    Static =     1<<0,
    DeviceOnly = 1<<1,
    Vertex =     1<<2,
    Fragment =   1<<3,
    Compute =    1<<4
  };
};

typedef std::vector<std::unique_ptr<Method>> MethodVector;

class EnumType : public Type {
 public:
  EnumType(std::string name);
  void                   Append(std::string identifier);
  void                   Append(std::string identifier, int value);
  bool                   IsEnum() const override { return true; }
  std::string            ToString() const override;
  int                    GetSizeInBytes() const override;
  const EnumValueVector& GetValues() { return values_; }
  const EnumValue*       FindValue(const std::string& id);
  std::string            GetName() { return name_; }
  bool                   CanWidenTo(Type* type) const override;

 private:
  EnumValueVector values_;
  int             nextValue_;
  std::string     name_;
};

class FormalTemplateArg : public Type {
 public:
  FormalTemplateArg(std::string name);
  std::string ToString() const override;
  bool IsFormalTemplateArg() const override { return true; }
  bool IsFullySpecified() const override { return false; }
  int  GetSizeInBytes() const override {
    assert(false);
    return 0;
  }
  std::string GetName() const { return name_; }

 private:
  std::string name_;
};

class UnresolvedScopedType : public Type {
 public:
  UnresolvedScopedType(FormalTemplateArg* baseType, std::string name);
  std::string ToString() const override;
  bool IsUnresolvedScopedType() const override { return true; }
  bool IsFullySpecified() const override { return false; }
  int  GetSizeInBytes() const override {
    assert(false);
    return 0;
  }
  FormalTemplateArg* GetBaseType() { return baseType_; }
  std::string        GetID() const { return id_; }

 private:
  FormalTemplateArg* baseType_;
  std::string        id_;
};

typedef std::vector<std::unique_ptr<EnumType>> EnumVector;

class ClassTemplate;

class ClassType : public Type {
 public:
  ClassType(std::string name);
  Field*              AddField(std::string name, Type* type, Expr* defaultValue);
  Field*              FindField(std::string name) const;
  void                AddMethod(Method* method);
  size_t              ComputeFieldOffsets();
  void                AddEnum(std::string id, EnumType* enumType);
  const FieldVector&  GetFields() const { return fields_; }          // local fields only
  int                 GetTotalFields() const { return numFields_; }  // includes inherited fields
  const MethodVector& GetMethods() { return methods_; }
  ClassTemplate*      GetTemplate() const { return template_; }
  const TypeList&     GetTemplateArgs() const { return templateArgs_; }
  std::string         ToString() const override;
  int                 GetSizeInBytes() const override;
  int                 GetSizeInBytes(int dynamicArrayLength) const override;
  int                 GetAlignmentInBytes() const override;
  bool                IsClass() const override { return true; }
  bool                CanWidenTo(Type* type) const override;
  bool                CanInitFrom(const ListType* type) const override;
  bool                IsUnsizedClass() const override;
  void                SetParent(ClassType* parent);
  void                SetTemplate(ClassTemplate* t) { template_ = t; }
  void        SetTemplateArgs(const TypeList& templateArgs) { templateArgs_ = templateArgs; }
  std::string GetName() const { return name_; }
  bool        IsDefined() const { return isDefined_; }
  void        SetDefined(bool defined) { isDefined_ = defined; }
  bool        HasNativeMethods() const;
  ClassType*  GetParent() const { return parent_; }
  Method*                     GetDestructor() { return destructor_; }
  Type*                       FindType(const std::string& id);
  void                        DefineType(std::string id, Type* type) { types_[id] = type; }
  void                        SetMemoryLayout(MemoryLayout memoryLayout, TypeTable* types);
  void                        SetMemoryLayout(MemoryLayout memoryLayout) { memoryLayout_ = memoryLayout; }
  MemoryLayout                GetMemoryLayout() const { return memoryLayout_; }
  int                         GetPadding() const { return padding_; }
  bool                        IsFullySpecified() const override;
  bool                        NeedsDestruction() const override;
  bool                        ContainsRawPtr() const override;
  const TypeMap&              GetTypes() { return types_; }

 private:
  std::string          name_;
  ClassType*           parent_ = nullptr;
  FieldVector          fields_;
  MethodVector         methods_;
  TypeMap              types_;
  EnumVector           enums_;
  ClassTemplate*       template_ = nullptr;
  TypeList             templateArgs_;
  Method*              destructor_ = nullptr;
  int                  numFields_ = 0;  // includes inherited fields
  bool                 isDefined_ = false;
  MemoryLayout         memoryLayout_ = MemoryLayout::Default;
  int                  padding_ = 0;
};

class ClassTemplate : public ClassType {
 public:
  ClassTemplate(std::string name, const TypeList& formalTemplateArgs);
  const TypeList&                GetFormalTemplateArgs() { return formalTemplateArgs_; }
  bool                           IsClassTemplate() const override { return true; }
  bool                           IsFullySpecified() const override { return false; }
  const std::vector<ClassType*>& GetInstances() const { return instances_; }
  void AddInstance(ClassType* instance) { instances_.push_back(instance); }

 private:
  TypeList                formalTemplateArgs_;
  std::vector<ClassType*> instances_;
};

class PtrType : public Type {
 public:
  PtrType(Type* type);
  bool  IsPtr() const override { return true; }
  Type* GetBaseType() const { return baseType_; }
  bool  IsFullySpecified() const override { return baseType_->IsFullySpecified(); }
  int   GetSizeInBytes() const override { return 2 * sizeof(void*); }
  bool  NeedsDestruction() const override { return true; }
  bool  ContainsRawPtr() const override { return baseType_->IsRawPtr(); }
 private:
  Type* baseType_;
};

class StrongPtrType : public PtrType {
 public:
  StrongPtrType(Type* type);
  std::string ToString() const override;
  bool        IsStrongPtr() const override { return true; }
  bool        CanWidenTo(Type* type) const override;
};

class WeakPtrType : public PtrType {
 public:
  WeakPtrType(Type* type);
  std::string ToString() const override;
  bool        IsWeakPtr() const override { return true; }
  bool        CanWidenTo(Type* type) const override;
  bool CanInitFrom(const ListType* type) const override { return GetBaseType()->CanInitFrom(type); }
};

class RawPtrType : public PtrType {
 public:
  RawPtrType(Type* type);
  std::string ToString() const override;
  bool        IsRawPtr() const override { return true; }
  bool        CanWidenTo(Type* type) const override;
  bool        CanInitFrom(const ListType* type) const override { return GetBaseType()->CanInitFrom(type); }
  int         GetSizeInBytes() const override { return sizeof(void*); }
  bool        ContainsRawPtr() const override { return true; }
};

class ListType : public Type {
 public:
  ListType(const VarVector& types);
  bool             IsList() const override { return true; }
  std::string      ToString() const override;
  bool             CanWidenTo(Type* type) const override { return type->CanInitFrom(this); }
  int              GetSizeInBytes() const override;
  const VarVector& GetTypes() const { return types_; }
  bool             IsNamed() const { return !types_.empty() && !types_[0]->name.empty(); }

 private:
  VarVector types_;
};

typedef std::vector<std::unique_ptr<Type>>     TypeStorageVector;
typedef std::vector<Type*>                     TypeVector;
typedef std::vector<std::unique_ptr<TypeList>> TypeListVector;

using TypeAndInt = std::pair<Type*, int>;
using TypeAndId = std::pair<Type*, std::string>;
using ArrayTypeKey = std::pair<TypeAndInt, MemoryLayout>;

class TypeTable {
 public:
  TypeTable();
  template <typename T, typename... ARGS>
  T* Make(ARGS&&... args) {
    T* type = new T(std::forward<ARGS>(args)...);
    typesStorage_.push_back(std::unique_ptr<T>(type));
    types_.push_back(type);
    return type;
  }
  BoolType*          GetBool();
  IntegerType*       GetInteger(int bits, bool isSigned);
  IntegerType*       GetByte();
  IntegerType*       GetUByte();
  IntegerType*       GetShort();
  IntegerType*       GetUShort();
  IntegerType*       GetInt();
  IntegerType*       GetUInt();
  FloatingPointType* GetFloatingPoint(int bits);
  FloatingPointType* GetFloat();
  FloatingPointType* GetDouble();
  VoidType*          GetVoid();
  AutoType*          GetAuto();
  ListType*          GetList(VarVector&& types);
  VectorType*        GetVector(Type* componentType, int size);
  MatrixType*        GetMatrix(VectorType* columnType, int numColumns);
  StrongPtrType*     GetStrongPtrType(Type* type);
  WeakPtrType*       GetWeakPtrType(Type* type);
  RawPtrType*        GetRawPtrType(Type* type);
  ArrayType*         GetArrayType(Type* elementType, int size, MemoryLayout layout);
  FormalTemplateArg* GetFormalTemplateArg(std::string name);
  ClassType*  GetClassTemplateInstance(ClassTemplate* classTemplate, const TypeList& templateArgs, std::queue<ClassType*>* instanceQueue);
  Type*       GetQualifiedType(Type* type, int qualifiers);
  Type*       GetUnresolvedScopedType(FormalTemplateArg* baseType, std::string id);
  TypeList*   AppendTypeList(TypeList* type);
  static bool VectorScalar(Type* lhs, Type* rhs);
  static bool ScalarVector(Type* lhs, Type* rhs);
  static bool MatrixScalar(Type* lhs, Type* rhs);
  static bool ScalarMatrix(Type* lhs, Type* rhs);
  static bool MatrixVector(Type* lhs, Type* rhs);
  static bool VectorMatrix(Type* lhs, Type* rhs);
  void        SetMemoryLayout();
  void        ComputeFieldOffsets();
  const TypeVector& GetTypes() { return types_; }

 private:
  TypeStorageVector                                    typesStorage_;
  TypeVector                                           types_;
  TypeListVector                                       typeLists_;
  std::unordered_map<int, IntegerType*>                integerTypes_;
  std::unordered_map<int, FloatingPointType*>          floatingPointTypes_;
  std::unordered_map<Type*, StrongPtrType*>            strongPtrTypes_;
  std::unordered_map<Type*, WeakPtrType*>              weakPtrTypes_;
  std::unordered_map<Type*, RawPtrType*>               rawPtrTypes_;
  std::unordered_map<ArrayTypeKey, ArrayType*>         arrayTypes_;
  std::unordered_map<TypeAndInt, VectorType*>          vectorTypes_;
  std::unordered_map<TypeAndInt, MatrixType*>          matrixTypes_;
  std::unordered_map<std::string, FormalTemplateArg*>  formalTemplateArgs_;
  std::unordered_map<TypeAndInt, QualifiedType*>       qualifiedTypes_;
  std::unordered_map<TypeAndId, UnresolvedScopedType*> unresolvedScopedTypes_;
  std::vector<ListType*>                               listTypes_;
  BoolType*                                            bool_;
  VoidType*                                            void_;
  AutoType*                                            auto_;
};

};  // namespace Toucan

#endif
