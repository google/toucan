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

#include "codegen_spirv.h"

#include <string.h>

#include <spirv/1.2/GLSL.std.450.h>
#include <spirv/unified1/spirv.hpp>

#include <ast/native_class.h>

#define NOTIMPLEMENTED() assert(false)

namespace {
const int WordCountShift = 16;
}

constexpr uint32_t kThis = 0xFFFFFFFF;
constexpr uint32_t kBuiltIns = 0xFFFFFFFE;
constexpr uint32_t kBindGroupsStart = 0xFFFFFFF0;
constexpr uint32_t kMaxBindGroups = 4;
constexpr uint32_t kBindGroupsEnd = kBindGroupsStart + kMaxBindGroups - 1;

namespace Toucan {

bool isBuffer(ClassType* classType) { return classType->GetTemplate() == NativeClass::Buffer; }

bool isColorAttachment(ClassType* classType) { return classType->GetTemplate() == NativeClass::ColorAttachment; }

bool isSampler(ClassType* classType) { return classType == NativeClass::Sampler; }

bool isSampleableTexture1D(ClassType* classType) {
  return classType->GetTemplate() == NativeClass::SampleableTexture1D;
}

bool isSampleableTexture2D(ClassType* classType) {
  return classType->GetTemplate() == NativeClass::SampleableTexture2D;
}

bool isSampleableTexture3D(ClassType* classType) {
  return classType->GetTemplate() == NativeClass::SampleableTexture3D;
}

bool isSampleableTexture2DArray(ClassType* classType) {
  return classType->GetTemplate() == NativeClass::SampleableTexture2DArray;
}

bool isSampleableTextureCube(ClassType* classType) {
  return classType->GetTemplate() == NativeClass::SampleableTextureCube;
}

bool isTextureView(ClassType* classType) {
  return isSampleableTexture1D(classType) || isSampleableTexture2D(classType) || isSampleableTexture3D(classType) ||
         isSampleableTexture2DArray(classType) || isSampleableTextureCube(classType);
}

bool isMath(ClassType* classType) { return classType == NativeClass::Math; }

bool isSystem(ClassType* classType) { return classType == NativeClass::System; }

bool isBuiltInClass(ClassType* classType) {
  return classType->GetName() == "FragmentBuiltins" || classType->GetName() == "VertexBuiltins" ||
         classType->GetName() == "ComputeBuiltins";
}

uint32_t builtinNameToID(const std::string& name) {
  if (name == "vertexIndex") {
    return spv::BuiltInVertexIndex;
  } else if (name == "instanceIndex") {
    return spv::BuiltInInstanceIndex;
  } else if (name == "position") {
    return spv::BuiltInPosition;
  } else if (name == "pointSize") {
    return spv::BuiltInPointSize;
  } else if (name == "fragCoord") {
    return spv::BuiltInFragCoord;
  } else if (name == "frontFacing") {
    return spv::BuiltInFrontFacing;
  } else if (name == "fragDepth") {
    return spv::BuiltInFragDepth;
  } else if (name == "localInvocationId") {
    return spv::BuiltInLocalInvocationId;
  } else if (name == "localInvocationIndex") {
    return spv::BuiltInLocalInvocationIndex;
  } else if (name == "globalInvocationId") {
    return spv::BuiltInGlobalInvocationId;
  } else if (name == "globalInvocationId") {
    return spv::BuiltInGlobalInvocationId;
  } else if (name == "workgroupId") {
    return spv::BuiltInWorkgroupId;
  } else if (name == "numWorkgroups") {
    return spv::BuiltInNumWorkgroups;
  } else if (name == "sampleIndex") {
    return spv::BuiltInSampleId;
  } else if (name == "sampleMaskIn" || name == "sampleMaskOut") {
    return spv::BuiltInSampleMask;
  } else {
    assert(false);
    return 0;
  }
}

spv::Op binOpToOpcode(BinOpNode::Op op,
                      Type*         lhsType,
                      Type*         rhsType,
                      uint32_t*     lhs,
                      uint32_t*     rhs) {
  bool isFloat = lhsType->IsFloat() || lhsType->IsFloatVector();
  bool isBool = lhsType->IsBool();
  switch (op) {
    case BinOpNode::ADD: return isFloat ? spv::OpFAdd : spv::OpIAdd;
    case BinOpNode::SUB: return isFloat ? spv::OpFSub : spv::OpISub;
    case BinOpNode::MUL:
      if (TypeTable::VectorScalar(lhsType, rhsType)) {
        return spv::OpVectorTimesScalar;
      } else if (TypeTable::ScalarVector(lhsType, rhsType)) {
        std::swap(*lhs, *rhs);
        return spv::OpVectorTimesScalar;
      } else if (TypeTable::MatrixVector(lhsType, rhsType)) {
        return spv::OpMatrixTimesVector;
      } else if (TypeTable::VectorMatrix(lhsType, rhsType)) {
        return spv::OpVectorTimesMatrix;
      } else if (lhsType->IsMatrix() && rhsType->IsMatrix()) {
        return spv::OpMatrixTimesMatrix;
      } else {
        return isFloat ? spv::OpFMul : spv::OpIMul;
      }
      break;
    case BinOpNode::DIV: return isFloat ? spv::OpFDiv : spv::OpSDiv; break;
    case BinOpNode::MOD: return spv::OpSRem; break;
    case BinOpNode::LT: return isFloat ? spv::OpFOrdLessThan : spv::OpSLessThan; break;
    case BinOpNode::LE: return isFloat ? spv::OpFOrdLessThanEqual : spv::OpSLessThanEqual; break;
    case BinOpNode::EQ:
      return isFloat ? spv::OpFOrdEqual : isBool ? spv::OpLogicalEqual : spv::OpIEqual;
      break;
    case BinOpNode::GE:
      return isFloat ? spv::OpFOrdGreaterThanEqual : spv::OpSGreaterThanEqual;
      break;
    case BinOpNode::GT: return isFloat ? spv::OpFOrdGreaterThan : spv::OpSGreaterThan; break;
    case BinOpNode::NE:
      return isFloat ? spv::OpFOrdNotEqual : isBool ? spv::OpLogicalNotEqual : spv::OpINotEqual;
      break;
    case BinOpNode::LOGICAL_AND: return spv::OpLogicalAnd;
    case BinOpNode::LOGICAL_OR: return spv::OpLogicalOr;
    default: NOTIMPLEMENTED();
  }
  return spv::OpNop;
}

class VarDeclVisitor : public Visitor {
 public:
  VarDeclVisitor(CodeGenSPIRV* codegen) : codegen_(codegen) {}
  Result Default(ASTNode*) override { return {}; }
  Result Visit(Stmts* stmts) override {
    for (auto var : stmts->GetVars()) {
      codegen_->DeclareVar(var.get());
    }
    for (Stmt* const& i : stmts->GetStmts()) {
      i->Accept(this);
    }
    return {};
  }
  Result Visit(IfStatement* stmt) override {
    stmt->GetStmt()->Accept(this);
    if (stmt->GetOptElse()) stmt->GetOptElse()->Accept(this);
    return {};
  }
  Result Visit(WhileStatement* stmt) override {
    if (stmt->GetBody()) stmt->GetBody()->Accept(this);
    return {};
  }
  Result Visit(DoStatement* stmt) override {
    if (stmt->GetBody()) stmt->GetBody()->Accept(this);
    return {};
  }
  Result Visit(ForStatement* stmt) override {
    if (stmt->GetInitStmt()) stmt->GetInitStmt()->Accept(this);
    stmt->GetBody()->Accept(this);
    return {};
  }
  CodeGenSPIRV* codegen_;
};

spv::ExecutionModel toExecutionModel(ShaderType shaderType) {
  if (shaderType == ShaderType::Vertex) {
    return spv::ExecutionModelVertex;
  } else if (shaderType == ShaderType::Fragment) {
    return spv::ExecutionModelFragment;
  } else if (shaderType == ShaderType::Compute) {
    return spv::ExecutionModelGLCompute;
  }
  assert(false);
  return spv::ExecutionModelFragment;
}

uint32_t CodeGenSPIRV::DeclareAndLoadInputVar(Type*      type,
                                              ShaderType shaderType,
                                              Code*      interface,
                                              uint32_t   location) {
  uint32_t ptrToType = ConvertPointerToType(type, spv::StorageClassInput);
  uint32_t ptrId = AppendDecl(spv::Op::OpVariable, ptrToType, {spv::StorageClassInput});
  interface->push_back(ptrId);
  uint32_t varId = AppendCode(spv::Op::OpLoad, ConvertType(type), {ptrId});
  Append(spv::OpDecorate, {ptrId, spv::DecorationLocation, location}, &annotations_);
  if (shaderType == ShaderType::Fragment && type->IsInteger()) {
    Append(spv::OpDecorate, {ptrId, spv::DecorationFlat}, &annotations_);
  }
  return varId;
}

uint32_t CodeGenSPIRV::GetStorageClass(Type* type) {
  if (type->IsRawPtr()) { type = static_cast<RawPtrType*>(type)->GetBaseType(); }
  int qualifiers;
  type = types_->GetUnqualifiedType(type, &qualifiers);
  if (type->IsClass()) {
    auto classType = static_cast<ClassType*>(type);
    if (isSampler(classType) || isTextureView(classType)) {
      return spv::StorageClassUniformConstant;
    }
  }
  if (qualifiers & Type::Qualifier::Uniform) {
    return spv::StorageClassUniform;
  } else if (qualifiers & Type::Qualifier::Storage) {
    return spv::StorageClassStorageBuffer;
  } else if (type->IsUnsizedArray()) {
    return spv::StorageClassStorageBuffer;
  }
  return spv::StorageClassFunction;
}

void CodeGenSPIRV::DeclareBuiltInVars(Type* type, Code* interface) {
  assert(type->IsClass());
  auto classType = static_cast<ClassType*>(type);
  for (const auto& field : classType->GetFields()) {
    uint32_t storageClass =
        field->type->IsWriteable() ? spv::StorageClassOutput : spv::StorageClassInput;
    uint32_t typeId = ConvertPointerToType(field->type, storageClass);
    uint32_t ptrId = AppendDecl(spv::Op::OpVariable, typeId, {storageClass});
    uint32_t builtinId = builtinNameToID(field->name);
    Append(spv::OpDecorate, {ptrId, spv::DecorationBuiltIn, builtinId}, &annotations_);
    auto var = std::make_shared<Var>(field->name, field->type);
    var->spirv = ptrId;
    builtInVars_.push_back(var);
    interface->push_back(ptrId);
  }
}

// Given a struct, declares Input globals for each of its fields,
// Loads the globals and Constructs in instance of the struct from the vars.
uint32_t CodeGenSPIRV::DeclareAndLoadInputVars(Type* type, ShaderType shaderType, Code* interface) {
  if (type->IsClass()) {
    auto     classType = static_cast<ClassType*>(type);
    Code     args;
    uint32_t location = 0;
    for (auto& i : classType->GetFields()) {
      Field* field = i.get();
      args.push_back(DeclareAndLoadInputVar(field->type, shaderType, interface, location));
      location++;
    }
    return AppendCode(spv::Op::OpCompositeConstruct, ConvertType(type), args);
  } else {
    return DeclareAndLoadInputVar(type, shaderType, interface, 0);
  }
}

void CodeGenSPIRV::DeclareAndStoreOutputVar(Type*      type,
                                            uint32_t   valueId,
                                            ShaderType shaderType,
                                            Code*      interface,
                                            uint32_t   location) {
  uint32_t ptrToType = ConvertPointerToType(type, spv::StorageClassOutput);
  uint32_t ptrId = AppendDecl(spv::Op::OpVariable, ptrToType, {spv::StorageClassOutput});
  AppendCode(spv::Op::OpStore, {ptrId, valueId});
  interface->push_back(ptrId);
  Append(spv::OpDecorate, {ptrId, spv::DecorationLocation, location}, &annotations_);
  if (shaderType == ShaderType::Vertex && type->IsInteger()) {
    Append(spv::OpDecorate, {ptrId, spv::DecorationFlat}, &annotations_);
  }
}

// Given a type, declares corresponding Input or Output globals. For class types, do so for
// each of its fields.
// Also builds Loads (for Input) or Stores (for output) to populate an instance of the struct
// from the IO vars.
void CodeGenSPIRV::DeclareAndStoreOutputVars(Type*      type,
                                             uint32_t   valueId,
                                             ShaderType shaderType,
                                             Code*      interface) {
  if (type->IsVoid()) { return; }
  if (type->IsClass()) {
    uint32_t location = 0;
    auto     classType = static_cast<ClassType*>(type);
    for (auto& i : classType->GetFields()) {
      Field*   field = i.get();
      uint32_t type = ConvertType(field->type);
      uint32_t fieldId = AppendCode(spv::Op::OpCompositeExtract, type, {valueId, location});
      DeclareAndStoreOutputVar(field->type, fieldId, shaderType, interface, location);
      location++;
    }
  } else {
    DeclareAndStoreOutputVar(type, valueId, shaderType, interface, 0);
  }
}

Type* CodeGenSPIRV::GetAndQualifyUnderlyingType(Type* type) {
  assert(type->IsPtr());
  type = static_cast<PtrType*>(type)->GetBaseType();
  int   qualifiers;
  Type* unqualifiedType = types_->GetUnqualifiedType(type, &qualifiers);
  assert(unqualifiedType->IsClass());
  ClassType* classType = static_cast<ClassType*>(unqualifiedType);
  if (isBuffer(classType)) {
    assert(classType->GetTemplateArgs().size() == 1);
    Type* templateArgType = classType->GetTemplateArgs()[0];
    type = types_->GetQualifiedType(templateArgType, qualifiers);
    if (templateArgType->IsUnsizedArray()) {
      type = types_->GetWrapperClass(type);
      type = types_->GetQualifiedType(type, qualifiers);
    }
    return type;
  }
  return type;
}

Type* CodeGenSPIRV::GetSampledType(ClassType* colorAttachment) {
  Type* argType = colorAttachment->GetTemplateArgs()[0];
  Type* sampledType = static_cast<ClassType*>(argType)->FindType("SampledType");
  return types_->GetVector(sampledType, 4);
}

uint32_t CodeGenSPIRV::GetSampledImageType(Type* type) {
  auto t = sampledImageTypes_.find(type);
  if (t != sampledImageTypes_.end()) { return t->second; }
  uint32_t sampledImageType = AppendTypeDecl(spv::OpTypeSampledImage, {ConvertType(type)});
  sampledImageTypes_[type] = sampledImageType;
  return sampledImageType;
}

void CodeGenSPIRV::ExtractPipelineVars(Method* entryPoint, Code* interface) {
  if (entryPoint->modifiers & Method::STATIC) { return; }
  assert(entryPoint->formalArgList.size() > 0);
  thisPtrType_ = static_cast<PtrType*>(entryPoint->formalArgList[0]->type);
  assert(thisPtrType_->IsPtr());
  ExtractPipelineVars(entryPoint->classType, entryPoint->shaderType, interface);
  assert(bindGroups_.size() <= kMaxBindGroups);
  // Remove this "this" pointer.
  entryPoint->formalArgList.erase(entryPoint->formalArgList.begin());
  entryPoint->modifiers |= Method::STATIC;
}

void CodeGenSPIRV::ExtractPipelineVars(ClassType* classType, ShaderType shaderType, Code* interface) {
  if (classType->GetParent()) { ExtractPipelineVars(classType->GetParent(), shaderType, interface); }
  uint32_t outputLocation = 0;
  for (const auto& field : classType->GetFields()) {
    Type* type = field->type;
    uint32_t pipelineVar = 0;

    // TODO: eventually, everything should be ptrs
    if (type->IsPtr()) { type = static_cast<PtrType*>(type)->GetBaseType(); }
    Type* unqualifiedType = type->GetUnqualifiedType();
    assert(unqualifiedType->IsClass());
    ClassType* classType = static_cast<ClassType*>(unqualifiedType);
    if (classType->GetTemplate() == NativeClass::ColorAttachment) {
      if (shaderType == ShaderType::Fragment) {
        Type*    sampledType = GetSampledType(classType);
        uint32_t ptrToType = ConvertPointerToType(sampledType, spv::StorageClassOutput);
        uint32_t ptrId = AppendDecl(spv::Op::OpVariable, ptrToType, {spv::StorageClassOutput});
        interface->push_back(ptrId);
        Append(spv::OpDecorate, {ptrId, spv::DecorationLocation, outputLocation++}, &annotations_);
        pipelineVar = ptrId;
      }
    } else if (classType->GetTemplate() == NativeClass::DepthStencilAttachment) {
    } else {
      VarVector bindGroup;
      if (field->type->IsClass()) {
        auto* bindGroupClass = static_cast<ClassType*>(field->type);
        for (auto& subField : bindGroupClass->GetFields()) {
          Type* type = GetAndQualifyUnderlyingType(subField->type);
          auto  var = std::make_shared<Var>(field->name, type);
          bindGroup.push_back(var);
        }
      } else {
        Type* type = GetAndQualifyUnderlyingType(field->type);
        auto  var = std::make_shared<Var>(field->name, type);
        bindGroup.push_back(var);
      }
      pipelineVar = kBindGroupsStart + bindGroups_.size();
      bindGroups_.push_back(bindGroup);
    }
    pipelineVars_.push_back(pipelineVar);
  }
}

void CodeGenSPIRV::Run(Method* entryPoint) {
  glslStd450Import_ = NextId();
  uint32_t functionId = NextId();
  // Note: we're going to butcher the arg list, so reference the old one for
  // the duration of this method.
  auto argsBackup = entryPoint->formalArgList;

  Code        interface;
  ExtractPipelineVars(entryPoint, &interface);
  uint32_t group = 0;
  for (auto& bindGroup : bindGroups_) {
    uint32_t binding = 0;
    for (auto& var : bindGroup) {
      uint32_t varId = DeclareVar(var.get());
      Append(spv::OpDecorate, {varId, spv::DecorationDescriptorSet, group}, &annotations_);
      Append(spv::OpDecorate, {varId, spv::DecorationBinding, binding}, &annotations_);
      int qualifiers;
      types_->GetUnqualifiedType(var->type, &qualifiers);
      if (qualifiers & Type::Qualifier::Coherent) {
        Append(spv::OpDecorate, {varId, spv::DecorationCoherent}, &annotations_);
      }
      binding++;
    }
    group++;
  }
  const auto& args = entryPoint->formalArgList;
  assert(args.size() >= 1 && args.size() <= 2);
  Var* builtins = args[0].get();
  DeclareBuiltInVars(builtins->type, &interface);
  // Keep only the inputs. Get rid of the Builtins arg
  entryPoint->formalArgList.erase(entryPoint->formalArgList.begin());
  GenCodeForMethod(entryPoint, functionId);
  // Generate the code for an entry point function which wraps the given Toucan function.
  // Turn all the arguments to the function into Input variables.
  uint32_t resultType = ConvertType(types_->GetVoid());
  uint32_t functionTypeId = GetFunctionType({resultType});
  uint32_t entryPointId =
      AppendCode(spv::Op::OpFunction, resultType, {spv::FunctionControlMaskNone, functionTypeId});
  AppendCode(spv::Op::OpLabel, {NextId()});
  uint32_t executionModel = toExecutionModel(entryPoint->shaderType);
  Code     functionArgs;
  functionArgs.push_back(functionId);
  if (args.size() > 0) {
    uint32_t inputId = DeclareAndLoadInputVars(args[0]->type, entryPoint->shaderType, &interface);
    functionArgs.push_back(inputId);
  }
  uint32_t returnValueId =
      AppendCode(spv::Op::OpFunctionCall, ConvertType(entryPoint->returnType), functionArgs);
  DeclareAndStoreOutputVars(entryPoint->returnType, returnValueId, entryPoint->shaderType,
                            &interface);
  AppendCode(spv::Op::OpReturn, {});
  AppendCode(spv::Op::OpFunctionEnd, {});
  for (const auto& it : functions_) {
    Method*  m = it.first;
    uint32_t functionId = it.second;
    GenCodeForMethod(m, functionId);
  }
  functions_.clear();

  header_.push_back(spv::MagicNumber);
  header_.push_back(0x00010300);
  header_.push_back(0);         // Generator
  header_.push_back(nextID_);   // Bound
  header_.push_back(0);         // Schema
  Append(spv::OpCapability, {spv::CapabilityMatrix}, &header_);
  Append(spv::OpCapability, {spv::CapabilityShader}, &header_);
  Append(spv::OpCapability, {spv::CapabilitySampled1D}, &header_);
  Append(spv::OpCapability, {spv::CapabilityImage1D}, &header_);
  Code importName;
  AppendString("GLSL.std.450", &importName);
  header_.push_back(spv::OpExtInstImport | ((2 + importName.size()) << WordCountShift));
  header_.push_back(glslStd450Import_);
  header_.insert(header_.end(), importName.begin(), importName.end());
  Append(spv::OpMemoryModel, {spv::AddressingModelLogical, spv::MemoryModelGLSL450}, &header_);
  AppendEntryPoint(executionModel, entryPointId, "main", interface);
  if (entryPoint->shaderType == ShaderType::Fragment) {
    Append(spv::OpExecutionMode, {entryPointId, spv::ExecutionModeOriginUpperLeft}, &header_);
  } else if (entryPoint->shaderType == ShaderType::Compute) {
    const uint32_t* ws = entryPoint->workgroupSize;
    Append(spv::OpExecutionMode, {entryPointId, spv::ExecutionModeLocalSize, ws[0], ws[1], ws[2]},
           &header_);
  }
}

uint32_t CodeGenSPIRV::GenerateSPIRV(ASTNode* node) { return node->Accept(this).i; }

Result CodeGenSPIRV::Visit(ArgList* list) {
  NOTIMPLEMENTED();
  return 0u;
}

Result CodeGenSPIRV::Visit(ArrayAccess* node) {
  Type* type = node->GetExpr()->GetType(types_);
  if (type->IsRawPtr()) { type = static_cast<RawPtrType*>(type)->GetBaseType(); }
  assert(type->IsArray());
  ArrayType* arrayType = static_cast<ArrayType*>(type);

  uint32_t storageClass = GetStorageClass(node->GetType(types_));
  uint32_t resultType = ConvertPointerToType(arrayType->GetElementType(), storageClass);
  uint32_t base = GenerateSPIRV(node->GetExpr());
  uint32_t index = GenerateSPIRV(node->GetIndex());
  if (type->IsUnsizedArray()) {
    base = AppendCode(spv::Op::OpAccessChain, ConvertPointerToType(arrayType),
                      {base, GetIntConstant(0)});
  }
  uint32_t resultId = AppendCode(spv::Op::OpAccessChain, resultType, {base, index});
  return resultId;
}

void CodeGenSPIRV::Append(uint32_t opCode, const Code& args, Code* result) {
  uint32_t wordCount = 1 + args.size();
  result->push_back((wordCount << WordCountShift) | opCode);
  for (uint32_t arg : args) {
    result->push_back(arg);
  }
}

uint32_t CodeGenSPIRV::Append(uint32_t    opCode,
                              uint32_t    resultType,
                              const Code& args,
                              Code*       result) {
  uint32_t wordCount = 3 + args.size();
  uint32_t resultId = NextId();
  result->push_back((wordCount << WordCountShift) | opCode);
  result->push_back(resultType);
  result->push_back(resultId);
  for (uint32_t arg : args) {
    result->push_back(arg);
  }
  return resultId;
}

uint32_t CodeGenSPIRV::AppendTypeDecl(uint32_t opCode, const Code& args) {
  uint32_t wordCount = 2 + args.size();
  uint32_t resultId = NextId();
  decl_.push_back((wordCount << WordCountShift) | opCode);
  decl_.push_back(resultId);
  for (uint32_t arg : args) {
    decl_.push_back(arg);
  }
  return resultId;
}

uint32_t CodeGenSPIRV::AppendString(const char* str, Code* result) {
  uint32_t    length = strlen(str);
  uint32_t    lengthInWords = length / 4 + 1;
  uint32_t    i = length;
  const char* s = str;
  for (; i > 3; i -= 4) {
    uint32_t v = *s | *(s + 1) << 8 | *(s + 2) << 16 | *(s + 3) << 24;
    s += 4;
    result->push_back(v);
  }
  uint32_t v = 0;
  for (uint32_t j = 0; j < i; j++) {
    v = v | (*s++ << (j * 8));
  }
  result->push_back(v);
  return lengthInWords;
}

void CodeGenSPIRV::AppendEntryPoint(uint32_t    executionModel,
                                    uint32_t    entryPoint,
                                    const char* name,
                                    const Code& interface) {
  Code     encodedName;
  uint32_t nameLengthInWords = AppendString(name, &encodedName);
  uint32_t wordCount = 3 + nameLengthInWords + interface.size();
  header_.push_back((wordCount << WordCountShift) | spv::OpEntryPoint);
  header_.push_back(executionModel);
  header_.push_back(entryPoint);
  header_.insert(header_.end(), encodedName.begin(), encodedName.end());
  header_.insert(header_.end(), interface.begin(), interface.end());
}

uint32_t CodeGenSPIRV::AppendCode(uint32_t opCode, uint32_t resultType, const Code& args) {
  return Append(opCode, resultType, args, &body_);
}

void CodeGenSPIRV::AppendCode(uint32_t opCode, const Code& args) { Append(opCode, args, &body_); }

uint32_t CodeGenSPIRV::AppendDecl(uint32_t opCode, uint32_t resultType, const Code& args) {
  return Append(opCode, resultType, args, &decl_);
}

uint32_t CodeGenSPIRV::AppendExtInst(uint32_t extInst, uint32_t resultType, ExprList* argList) {
  Code args;
  args.push_back(glslStd450Import_);
  args.push_back(extInst);
  for (auto& i : argList->Get()) {
    args.push_back(GenerateSPIRV(i));
  }
  return AppendCode(spv::Op::OpExtInst, resultType, args);
}

uint32_t CodeGenSPIRV::DeclareVar(Var* var) {
  if (var->type->IsPtr()) {
    // This variable will be removed by aliasing.
    return 0;
  }
  uint32_t storageClass = GetStorageClass(var->type);
  uint32_t varType = ConvertPointerToType(var->type, storageClass);
  uint32_t varId = AppendCode(spv::Op::OpVariable, varType, {storageClass});
  var->spirv = varId;
  return varId;
}

uint32_t CodeGenSPIRV::AppendImageDecl(uint32_t        dim,
                                       bool            array,
                                       int             qualifiers,
                                       const TypeList& templateArgs) {
  assert(templateArgs.size() >= 1);
  uint32_t sampledType = ConvertType(templateArgs[0]);
  uint32_t depth = 0;  // not depth
  uint32_t arrayed = array ? 1 : 0;
  uint32_t ms = 0;  // not multisampled
  uint32_t sampled = 1; // sampled
  uint32_t format = spv::ImageFormatUnknown; 
  return AppendTypeDecl(spv::Op::OpTypeImage,
                        {sampledType, dim, depth, arrayed, ms, sampled, format});
}

uint32_t CodeGenSPIRV::ConvertType(Type* type) {
  int qualifiers;
  type = types_->GetUnqualifiedType(type, &qualifiers);
  bool isInterfaceBlock = qualifiers & (Type::Qualifier::Uniform | Type::Qualifier::Storage);
  if (spirvTypes_[type] != 0) { return spirvTypes_[type]; }
  uint32_t resultId;
  if (type->IsPtr()) {
    Type* baseType = static_cast<PtrType*>(type)->GetBaseType();
    resultId = ConvertPointerToType(baseType);
  } else if (type->IsArray()) {
    ArrayType* a = static_cast<ArrayType*>(type);
    uint32_t   elementType = ConvertType(a->GetElementType());
    if (a->GetNumElements() == 0) {
      resultId = AppendTypeDecl(spv::Op::OpTypeRuntimeArray, {elementType});
    } else {
      uint32_t intType = ConvertType(types_->GetInt());
      uint32_t value = a->GetNumElements();
      uint32_t numElementsId = AppendDecl(spv::Op::OpConstant, intType, {value});
      resultId = AppendTypeDecl(spv::Op::OpTypeArray, {elementType, numElementsId});
    }
    Append(spv::OpDecorate,
           {resultId, spv::DecorationArrayStride, (uint32_t)a->GetElementSizeInBytes()},
           &annotations_);
  } else if (type->IsClass()) {
    ClassType* classType = static_cast<ClassType*>(type);
    if (classType->IsNative()) {
      if (isBuffer(classType)) {
        assert(classType->GetTemplateArgs().size() == 1);
        Type* baseType = classType->GetTemplateArgs()[0];
        resultId = ConvertPointerToType(baseType);
      } else if (isColorAttachment(classType)) {
        resultId = ConvertPointerToType(GetSampledType(classType));
      } else if (isSampler(classType)) {
        resultId = AppendTypeDecl(spv::Op::OpTypeSampler, {});
      } else if (isSampleableTexture1D(classType)) {
        resultId = AppendImageDecl(spv::Dim1D, false, qualifiers, classType->GetTemplateArgs());
      } else if (isSampleableTexture2D(classType)) {
        resultId = AppendImageDecl(spv::Dim2D, false, qualifiers, classType->GetTemplateArgs());
      } else if (isSampleableTexture3D(classType)) {
        resultId = AppendImageDecl(spv::Dim3D, false, qualifiers, classType->GetTemplateArgs());
      } else if (isSampleableTexture2DArray(classType)) {
        resultId = AppendImageDecl(spv::Dim2D, true, qualifiers, classType->GetTemplateArgs());
      } else if (isSampleableTextureCube(classType)) {
        resultId = AppendImageDecl(spv::DimCube, false, qualifiers, classType->GetTemplateArgs());
      } else {
        assert(!"unsupported native class type in shader");
      }
    } else {
      bool builtin = isBuiltInClass(classType);
      Code args;
      for (auto& field : classType->GetFields()) {
        uint32_t typeId;
        if (builtin) {
          if (field->type->IsReadable()) {
            typeId = ConvertType(field->type);
          } else if (field->type->IsWriteable()) {
            typeId = ConvertPointerToType(field->type, spv::StorageClassOutput);
          }
        } else {
          typeId = ConvertType(field->type);
        }
        args.push_back(typeId);
      }
      resultId = AppendTypeDecl(spv::Op::OpTypeStruct, args);
      uint32_t i = 0;
      for (auto& field : classType->GetFields()) {
        Append(spv::OpMemberDecorate,
               {resultId, i, spv::DecorationOffset, static_cast<uint32_t>(field->offset)},
               &annotations_);
        if (field->type->IsMatrix()) {
          auto matrixType = static_cast<MatrixType*>(field->type);
          Append(spv::OpMemberDecorate, {resultId, i, spv::DecorationMatrixStride, 16},
                 &annotations_);
          Append(spv::OpMemberDecorate, {resultId, i, spv::DecorationColMajor}, &annotations_);
        }
        ++i;
      }
      if (isInterfaceBlock) {
        Append(spv::OpDecorate, {resultId, spv::DecorationBlock}, &annotations_);
      }
    }
  } else if (type->IsInteger()) {
    IntegerType* integerType = static_cast<IntegerType*>(type);
    resultId = AppendTypeDecl(spv::Op::OpTypeInt, {static_cast<uint32_t>(integerType->GetBits()),
                                                   integerType->Signed() ? 1u : 0u});
  } else if (type->IsEnum()) {
    resultId = AppendTypeDecl(spv::Op::OpTypeInt, {32, 0});
  } else if (type->IsFloat()) {
    resultId = AppendTypeDecl(spv::Op::OpTypeFloat, {32});
  } else if (type->IsBool()) {
    resultId = AppendTypeDecl(spv::Op::OpTypeBool, {});
  } else if (type->IsVector()) {
    VectorType* v = static_cast<VectorType*>(type);
    uint32_t    componentType = ConvertType(v->GetComponentType());
    resultId = AppendTypeDecl(spv::Op::OpTypeVector, {componentType, v->GetLength()});
  } else if (type->IsMatrix()) {
    auto*    m = static_cast<MatrixType*>(type);
    uint32_t columnType = ConvertType(m->GetColumnType());
    resultId = AppendTypeDecl(spv::Op::OpTypeMatrix, {columnType, m->GetNumColumns()});
  } else if (type->IsVoid()) {
    resultId = AppendTypeDecl(spv::Op::OpTypeVoid, {});
  } else {
    assert("ConvertType:  unknown type" == 0);
    return 0;
  }
  spirvTypes_[type] = resultId;
  return resultId;
}

uint32_t CodeGenSPIRV::GetFunctionType(const Code& signature) {
  if (spirvFunctionTypes_[signature] != 0) { return spirvFunctionTypes_[signature]; }
  uint32_t resultId = AppendTypeDecl(spv::Op::OpTypeFunction, signature);
  spirvFunctionTypes_[signature] = resultId;
  return resultId;
}

void CodeGenSPIRV::GenCodeForMethod(Method* method, uint32_t resultId) {
  uint32_t resultType = ConvertType(method->returnType);
  Code     argTypes{resultType};
  for (auto arg : method->formalArgList) {
    argTypes.push_back(ConvertType(arg->type));
  }
  uint32_t typeId = GetFunctionType(argTypes);
  AppendCode(spv::Op::OpFunction, {resultType, resultId, spv::FunctionControlMaskNone, typeId});
  Code fps;
  for (auto arg : method->formalArgList) {
    uint32_t resultType = ConvertType(arg->type);
    uint32_t fp = AppendCode(spv::Op::OpFunctionParameter, resultType, {});
    fps.push_back(fp);
  }
  AppendCode(spv::Op::OpLabel, {NextId()});
  assert(!method->classType->IsNative());

  if (method->stmts) {
    // Create function-local storage for any non-pointer arguments.
    for (auto arg : method->formalArgList) {
      if (!arg->type->IsPtr()) { DeclareVar(arg.get()); }
    }
    VarDeclVisitor varDeclVisitor(this);
    method->stmts->Accept(&varDeclVisitor);
    auto fp = fps.begin();
    // Store non-pointer arguments into function-local variables.
    for (auto arg : method->formalArgList) {
      if (!arg->type->IsPtr()) {
        AppendCode(spv::Op::OpStore, {arg->spirv, *fp++});
      } else {
        arg->spirv = *fp++;
      }
    }
    GenerateSPIRV(method->stmts);
  }

  AppendCode(spv::Op::OpFunctionEnd, {});
}

uint32_t CodeGenSPIRV::ConvertPointerToType(Type* type) {
  return ConvertPointerToType(type, GetStorageClass(type));
}

uint32_t CodeGenSPIRV::ConvertPointerToType(Type* type, uint32_t storageClass) {
  if (type->IsPtr()) { type = static_cast<PtrType*>(type)->GetBaseType(); }
  PtrTypeKey key(types_->GetUnqualifiedType(type), storageClass);
  if (spirvPtrTypes_[key] != 0) { return spirvPtrTypes_[key]; }
  uint32_t typeId = ConvertType(type);
  uint32_t resultId = AppendTypeDecl(spv::Op::OpTypePointer, {storageClass, typeId});
  spirvPtrTypes_[key] = resultId;
  return resultId;
}

uint32_t CodeGenSPIRV::CreateVectorSplat(uint32_t value, VectorType* type) {
  uint32_t resultType = ConvertType(type);
  Code     args;
  for (int i = 0; i < type->GetLength(); ++i) {
    args.push_back(value);
  }
  return AppendCode(spv::Op::OpCompositeConstruct, resultType, args);
}

Result CodeGenSPIRV::Visit(BinOpNode* node) {
  uint32_t lhs = GenerateSPIRV(node->GetLHS());
  uint32_t rhs = GenerateSPIRV(node->GetRHS());
  Type*    lhsType = node->GetLHS()->GetType(types_);
  Type*    rhsType = node->GetRHS()->GetType(types_);
  if (node->GetOp() == BinOpNode::DIV && lhsType != rhsType) {
    if (TypeTable::VectorScalar(lhsType, rhsType)) {
      rhs = CreateVectorSplat(rhs, static_cast<VectorType*>(lhsType));
    } else {
      assert(false);
    }
  }
  uint32_t typeId = ConvertType(node->GetType(types_));
  spv::Op  opCode = binOpToOpcode(node->GetOp(), lhsType, rhsType, &lhs, &rhs);
  uint32_t resultId = AppendCode(opCode, typeId, {lhs, rhs});
  return resultId;
}

Result CodeGenSPIRV::Visit(UnaryOp* node) {
  uint32_t rhs = GenerateSPIRV(node->GetRHS());
  Type*    rhsType = node->GetRHS()->GetType(types_);
  spv::Op  opCode = spv::OpNop;
  bool     isFloat = rhsType->IsFloat() || rhsType->IsFloatVector();
  switch (node->GetOp()) {
    case UnaryOp::Op::Minus: opCode = isFloat ? spv::OpFNegate : spv::OpSNegate; break;
    case UnaryOp::Op::Negate: opCode = spv::OpLogicalNot; break;
    default: NOTIMPLEMENTED(); break;
  }

  uint32_t typeId = ConvertType(node->GetType(types_));
  uint32_t resultId = AppendCode(opCode, typeId, {rhs});
  return resultId;
}

Result CodeGenSPIRV::Visit(BoolConstant* expr) {
  uint32_t resultType = ConvertType(expr->GetType(types_));
  uint32_t op = expr->GetValue() ? spv::Op::OpConstantTrue : spv::Op::OpConstantFalse;
  uint32_t resultId = AppendDecl(op, resultType, {});
  return resultId;
}

uint32_t CodeGenSPIRV::CreateCast(Type*    srcType,
                                  Type*    dstType,
                                  uint32_t resultType,
                                  uint32_t valueId) {
  if (dstType == srcType) {
    return valueId;
  } else if (dstType->IsFloat() && srcType->IsInt()) {
    return AppendCode(spv::Op::OpConvertSToF, resultType, {valueId});
  } else if (dstType->IsInt() && srcType->IsFloat()) {
    return AppendCode(spv::Op::OpConvertFToS, resultType, {valueId});
  } else if (dstType->IsInt() && srcType->IsUInt()) {
    return valueId;
  } else if (dstType->IsUInt() && srcType->IsInt()) {
    return valueId;
  } else if (dstType->IsVector() && srcType->IsVector()) {
    VectorType* t1 = static_cast<VectorType*>(dstType);
    VectorType* t2 = static_cast<VectorType*>(srcType);
    if (t1->GetLength() == t2->GetLength()) {
      return CreateCast(t1->GetComponentType(), t2->GetComponentType(), resultType, valueId);
    } else {
      assert(false);
    }
  } else if (dstType->IsUnsizedArray() && srcType->IsUnsizedArray()) {
    NOTIMPLEMENTED();
  } else if (dstType->IsPtr() && srcType->IsPtr()) {
    return valueId;
  }
  assert(false);
  return valueId;
}

Result CodeGenSPIRV::Visit(CastExpr* expr) {
  Type*    srcType = expr->GetExpr()->GetType(types_);
  Type*    dstType = expr->GetType();
  uint32_t resultType = ConvertType(dstType);
  uint32_t valueId = GenerateSPIRV(expr->GetExpr());
  return CreateCast(srcType, dstType, resultType, valueId);
}

Result CodeGenSPIRV::Visit(ConstructorNode* node) {
  const std::vector<Arg*>& args = node->GetArgList()->GetArgs();
  Code                     resultArgs;
  uint32_t                 resultType = ConvertType(node->GetType());
  for (Arg* const& arg : args) {
    resultArgs.push_back(GenerateSPIRV(arg->GetExpr()));
  }
  if (node->GetType()->IsVector()) {
    auto     vectorType = static_cast<VectorType*>(node->GetType());
    uint32_t zero = GetZeroConstant(vectorType->GetComponentType());
    for (int i = args.size(); i < vectorType->GetLength(); ++i) {
      resultArgs.push_back(zero);
    }
  }
  uint32_t resultId = AppendCode(spv::Op::OpCompositeConstruct, resultType, {resultArgs});
  return resultId;
}

Result CodeGenSPIRV::Visit(AddressOf* node) { return GenerateSPIRV(node->GetExpr()); }

Result CodeGenSPIRV::Visit(SmartToRawPtr* node) { return GenerateSPIRV(node->GetExpr()); }

Result CodeGenSPIRV::Visit(DoStatement* doStmt) {
  uint32_t loopBody = NextId();
  uint32_t next = NextId();
  uint32_t exitLoop = NextId();
  AppendCode(spv::Op::OpBranch, {loopBody});
  AppendCode(spv::Op::OpLabel, {loopBody});
  AppendCode(spv::Op::OpLoopMerge, {exitLoop, next, 0});
  AppendCode(spv::Op::OpBranch, {next});
  AppendCode(spv::Op::OpLabel, {next});
  GenerateSPIRV(doStmt->GetBody());
  Expr*    cond = doStmt->GetCond();
  uint32_t condition = GenerateSPIRV(cond);
  AppendCode(spv::Op::OpBranchConditional, {condition, loopBody, exitLoop});
  AppendCode(spv::Op::OpLabel, {exitLoop});
  return 0u;
}

Result CodeGenSPIRV::Visit(ExprStmt* exprStmt) {
  GenerateSPIRV(exprStmt->GetExpr());
  return 0u;
}

uint32_t CodeGenSPIRV::GetIntConstant(int32_t value) {
  if (intConstants_[value]) { return intConstants_[value]; }
  uint32_t resultType = ConvertType(types_->GetInt());
  uint32_t ivalue = static_cast<uint32_t>(value);
  uint32_t resultId = AppendDecl(spv::Op::OpConstant, resultType, {ivalue});
  intConstants_[value] = resultId;
  return resultId;
}

uint32_t CodeGenSPIRV::GetFloatConstant(float value) {
  if (floatConstants_[value]) { return floatConstants_[value]; }
  uint32_t resultType = ConvertType(types_->GetFloat());
  uint32_t ivalue = *reinterpret_cast<int*>(&value);
  uint32_t resultId = AppendDecl(spv::Op::OpConstant, resultType, {ivalue});
  floatConstants_[value] = resultId;
  return resultId;
}

uint32_t CodeGenSPIRV::GetZeroConstant(Type* type) {
  if (zeroConstants_[type]) { return zeroConstants_[type]; }
  uint32_t resultType = ConvertType(type);
  uint32_t resultId = AppendDecl(spv::Op::OpConstant, resultType, {0});
  zeroConstants_[type] = resultId;
  return resultId;
}

Result CodeGenSPIRV::Visit(FieldAccess* expr) {
  uint32_t base = GenerateSPIRV(expr->GetExpr());
  Field*   field = expr->GetField();

  if (base == kThis) {
    // Base node is "this" pointer. The result is a pipeline var.
    return pipelineVars_[field->index];
  } else if (base >= kBindGroupsStart && base <= kBindGroupsEnd) {
    // Base node is a bind group. Result is an interface variable.
    int group = base - kBindGroupsStart;
    assert(group >= 0);
    assert(group < kMaxBindGroups);
    Var* var = bindGroups_[group][field->index].get();
    return var->spirv;
  } else if (base == kBuiltIns) {
    return builtInVars_[field->index]->spirv;
  }

  uint32_t storageClass = GetStorageClass(expr->GetType(types_));
  uint32_t resultType = ConvertPointerToType(field->type, storageClass);
  uint32_t index = GetIntConstant(field->index);
  uint32_t resultId = AppendCode(spv::Op::OpAccessChain, resultType, {base, index});
  return resultId;
}

Result CodeGenSPIRV::Visit(FloatConstant* expr) {
  uint32_t resultType = ConvertType(expr->GetType(types_));
  float    value = expr->GetValue();
  uint32_t ivalue = *reinterpret_cast<int*>(&value);
  uint32_t resultId = AppendDecl(spv::Op::OpConstant, resultType, {ivalue});
  return resultId;
}

Result CodeGenSPIRV::Visit(ForStatement* forStmt) {
  Stmt* initStmt = forStmt->GetInitStmt();
  Expr* cond = forStmt->GetCond();
  Stmt* loopStmt = forStmt->GetLoopStmt();
  Stmt* body = forStmt->GetBody();

  if (initStmt) GenerateSPIRV(initStmt);
  uint32_t conditionLabel = NextId();
  uint32_t next = NextId();
  uint32_t loopBody = NextId();
  uint32_t continueLabel = NextId();
  uint32_t exitLoop = NextId();
  AppendCode(spv::Op::OpBranch, {conditionLabel});
  AppendCode(spv::Op::OpLabel, {conditionLabel});
  AppendCode(spv::Op::OpLoopMerge, {exitLoop, continueLabel, 0});
  AppendCode(spv::Op::OpBranch, {next});
  AppendCode(spv::Op::OpLabel, {next});
  if (cond) {
    uint32_t condition = GenerateSPIRV(cond);
    AppendCode(spv::Op::OpBranchConditional, {condition, loopBody, exitLoop});
  } else {
    AppendCode(spv::Op::OpBranch, {loopBody});
  }
  AppendCode(spv::Op::OpLabel, {loopBody});
  if (body) GenerateSPIRV(body);
  AppendCode(spv::Op::OpBranch, {continueLabel});
  AppendCode(spv::Op::OpLabel, {continueLabel});
  if (loopStmt) GenerateSPIRV(loopStmt);
  AppendCode(spv::Op::OpBranch, {conditionLabel});
  AppendCode(spv::Op::OpLabel, {exitLoop});
  return 0u;
}

Result CodeGenSPIRV::Visit(IfStatement* stmt) {
  Stmt* optElse = stmt->GetOptElse();

  uint32_t trueLabel = NextId();
  uint32_t falseLabel = NextId();
  uint32_t exitLabel = optElse ? NextId() : falseLabel;
  uint32_t conditionId = GenerateSPIRV(stmt->GetExpr());
  AppendCode(spv::Op::OpBranchConditional, {conditionId, trueLabel, falseLabel});
  AppendCode(spv::Op::OpLabel, {trueLabel});
  GenerateSPIRV(stmt->GetStmt());
  if (!stmt->GetStmt()->ContainsReturn()) { AppendCode(spv::Op::OpBranch, {exitLabel}); }
  if (optElse) {
    AppendCode(spv::Op::OpLabel, {falseLabel});
    GenerateSPIRV(optElse);
    if (!optElse->ContainsReturn()) { AppendCode(spv::Op::OpBranch, {exitLabel}); }
  }
  AppendCode(spv::Op::OpLabel, {exitLabel});
  return 0u;
}

Result CodeGenSPIRV::Visit(IntConstant* expr) {
  uint32_t resultId = GetIntConstant(static_cast<uint32_t>(expr->GetValue()));
  return resultId;
}

Result CodeGenSPIRV::Visit(UIntConstant* expr) {
  uint32_t resultId = GetIntConstant(expr->GetValue());
  return resultId;
}

Result CodeGenSPIRV::Visit(LengthExpr* expr) {
  NOTIMPLEMENTED();
  return 0u;
}

Result CodeGenSPIRV::Visit(NewArrayExpr* expr) {
  NOTIMPLEMENTED();
  return 0u;
}

Result CodeGenSPIRV::Visit(NewExpr* expr) {
  NOTIMPLEMENTED();
  return 0u;
}

Result CodeGenSPIRV::Visit(NullConstant* expr) {
  NOTIMPLEMENTED();
  return 0u;
}

Result CodeGenSPIRV::Visit(ReturnStatement* stmt) {
  Expr*    expr = stmt->GetExpr();
  uint32_t resultType = ConvertType(types_->GetVoid());
  if (expr) {
    AppendCode(spv::Op::OpReturnValue, {GenerateSPIRV(expr)});
  } else {
    AppendCode(spv::Op::OpReturn, {});
  }
  return 0u;
}

Result CodeGenSPIRV::Visit(MethodCall* expr) {
  Method*  method = expr->GetMethod();
  uint32_t resultType = ConvertType(expr->GetType(types_));
  if (isBuffer(method->classType)) {
    if (method->name == "MapReadUniform" || method->name == "MapReadStoreage" ||
        method->name == "MapWriteStorage" || method->name == "MapReadWriteStorage") {
      return GenerateSPIRV(expr->GetArgList()->Get().front());
    }
  } else if (isTextureView(method->classType)) {
    if (method->name == "Sample") {
      const std::vector<Expr*>& args = expr->GetArgList()->Get();
      Type*    textureType = static_cast<PtrType*>(args[0]->GetType(types_))->GetBaseType();
      Type*    samplerType = static_cast<PtrType*>(args[1]->GetType(types_))->GetBaseType();
      uint32_t texture = GenerateSPIRV(args[0]);
      uint32_t sampler = GenerateSPIRV(args[1]);
      texture = AppendCode(spv::Op::OpLoad, ConvertType(textureType), {texture});
      sampler = AppendCode(spv::Op::OpLoad, ConvertType(samplerType), {sampler});
      uint32_t imageType = GetSampledImageType(textureType);
      uint32_t sampledImage = AppendCode(spv::OpSampledImage, imageType, {texture, sampler});
      uint32_t coord = GenerateSPIRV(args[2]);
      if (isSampleableTexture2DArray(method->classType)) {
        uint32_t layer = GenerateSPIRV(args[3]);
        layer = AppendCode(spv::Op::OpConvertUToF, ConvertType(types_->GetFloat()), {layer});
        uint32_t float3 = ConvertType(types_->GetVector(types_->GetFloat(), 3));
        uint32_t floatType = ConvertType(types_->GetFloat());
        uint32_t x = AppendCode(spv::Op::OpCompositeExtract, floatType, {coord, 0});
        uint32_t y = AppendCode(spv::Op::OpCompositeExtract, floatType, {coord, 1});
        coord = AppendCode(spv::Op::OpCompositeConstruct, float3, {x, y, layer});
      }
      return AppendCode(spv::Op::OpImageSampleImplicitLod, resultType, {sampledImage, coord});
    }
  } else if (isColorAttachment(method->classType)) {
    if (method->name == "Set") {
      const std::vector<Expr*>& args = expr->GetArgList()->Get();
      assert(args.size() == 2);
      uint32_t colorAttachment = GenerateSPIRV(args[0]);
      uint32_t valueId = GenerateSPIRV(args[1]);
      AppendCode(spv::Op::OpStore, {colorAttachment, valueId});
      return {};
    }
  } else if (isMath(method->classType)) {
    if (method->name == "sqrt") {
      return AppendExtInst(GLSLstd450Sqrt, resultType, expr->GetArgList());
    } else if (method->name == "sin") {
      return AppendExtInst(GLSLstd450Sin, resultType, expr->GetArgList());
    } else if (method->name == "cos") {
      return AppendExtInst(GLSLstd450Cos, resultType, expr->GetArgList());
    } else if (method->name == "abs") {
      return AppendExtInst(GLSLstd450FAbs, resultType, expr->GetArgList());
    } else if (method->name == "reflect") {
      return AppendExtInst(GLSLstd450Reflect, resultType, expr->GetArgList());
    } else if (method->name == "refract") {
      return AppendExtInst(GLSLstd450Refract, resultType, expr->GetArgList());
    } else if (method->name == "normalize") {
      return AppendExtInst(GLSLstd450Normalize, resultType, expr->GetArgList());
    } else if (method->name == "inverse") {
      return AppendExtInst(GLSLstd450MatrixInverse, resultType, expr->GetArgList());
    } else if (method->name == "transpose") {
      Code args;
      for (auto& i : expr->GetArgList()->Get()) {
        args.push_back(GenerateSPIRV(i));
      }
      return AppendCode(spv::Op::OpTranspose, resultType, args);
    }
  } else if (isSystem(method->classType)) {
    if (method->name == "StorageBarrier") {
      Code args;
      args.push_back(GetIntConstant(spv::ScopeWorkgroup));
      args.push_back(GetIntConstant(spv::ScopeDevice));
      args.push_back(GetIntConstant(spv::MemorySemanticsUniformMemoryMask |
                                    spv::MemorySemanticsAcquireReleaseMask));
      AppendCode(spv::Op::OpControlBarrier, args);
      return {};  // FIXME: handle void method returns
    }
  }
  uint32_t functionId = functions_[method];
  if (functionId == 0) {
    functionId = NextId();
    functions_[method] = functionId;
  }
  Code args;
  args.push_back(functionId);
  for (auto& i : expr->GetArgList()->Get()) {
    args.push_back(GenerateSPIRV(i));
  }
  uint32_t resultId = AppendCode(spv::Op::OpFunctionCall, resultType, args);
  return resultId;
}

Result CodeGenSPIRV::Visit(Stmts* stmts) {
  for (Stmt* const& i : stmts->GetStmts()) {
    GenerateSPIRV(i);
  }
  return 0u;
}

Result CodeGenSPIRV::Visit(ExtractElementExpr* expr) {
  uint32_t resultType = ConvertType(expr->GetType(types_));
  uint32_t compositeId = GenerateSPIRV(expr->GetExpr());
  uint32_t index = expr->GetIndex();
  uint32_t resultId = AppendCode(spv::Op::OpCompositeExtract, resultType, {compositeId, index});
  return resultId;
}

Result CodeGenSPIRV::Visit(InsertElementExpr* expr) {
  uint32_t resultType = ConvertType(expr->GetType(types_));
  uint32_t compositeId = GenerateSPIRV(expr->GetExpr());
  uint32_t newElementId = GenerateSPIRV(expr->newElement());
  uint32_t index = expr->GetIndex();
  uint32_t resultId =
      AppendCode(spv::Op::OpCompositeInsert, resultType, {compositeId, newElementId, index});
  return resultId;
}

Result CodeGenSPIRV::Visit(VarExpr* expr) {
  Type* type = expr->GetVar()->type;

  // FIXME: why is the field argument being deref'ed?
  if (type == thisPtrType_ || thisPtrType_ && type == thisPtrType_->GetBaseType()) {
    return kThis;
  } else if (type->IsClass() && isBuiltInClass(static_cast<ClassType*>(type))) {
    return kBuiltIns;
  } else {
    // The storage for these is allocated by the VarDeclVisitor.
    return expr->GetVar()->spirv;
  }
}

Result CodeGenSPIRV::Visit(LoadExpr* expr) {
  uint32_t exprId = GenerateSPIRV(expr->GetExpr());
  uint32_t resultId;
  if (exprId == kThis) {
    resultId = kThis;
  } else if (exprId >= kBindGroupsStart && exprId <= kBindGroupsEnd) {
    // We're loading a single-entry bind group.
    Var* var = bindGroups_[exprId - kBindGroupsStart][0].get();
    resultId = var->spirv;
  } else {
    Type* type = expr->GetType(types_);
    // FIXME: this probably should've been handled elsewhere
    if (type->IsPtr()) {
      resultId = exprId;
    } else {
      uint32_t resultType = ConvertType(type);
      resultId = AppendCode(spv::Op::OpLoad, resultType, {exprId});
    }
  }
  return resultId;
}

class AliasVisitor : public Visitor {
 public:
  AliasVisitor(uint32_t reg) : reg_(reg) {}
  Result Default(ASTNode*) override {
    assert(false);
    return 0u;
  }
  Result Visit(VarExpr* expr) override {
    expr->GetVar()->spirv = reg_;
    return 0u;
  }

 private:
  uint32_t reg_;
};

Result CodeGenSPIRV::Visit(StoreStmt* stmt) {
  uint32_t objectId = GenerateSPIRV(stmt->GetRHS());
  if (stmt->GetRHS()->GetType(types_)->IsPtr()) {
    AliasVisitor aliasVisitor(objectId);
    stmt->GetLHS()->Accept(&aliasVisitor);
    return 0u;
  }
  uint32_t pointerId = GenerateSPIRV(stmt->GetLHS());
  AppendCode(spv::Op::OpStore, {pointerId, objectId});
  return 0u;
}

Result CodeGenSPIRV::Visit(IncDecExpr* node) {
  Type*    type = node->GetType(types_);
  uint32_t typeId = ConvertType(type);
  uint32_t ptr = GenerateSPIRV(node->GetExpr());
  uint32_t value = AppendCode(spv::Op::OpLoad, typeId, {ptr});
  uint32_t one;
  spv::Op  op;
  if (type->IsInteger()) {
    one = GetIntConstant(1);
    op = node->GetOp() == IncDecExpr::Op::Inc ? spv::OpIAdd : spv::OpISub;
  } else {
    one = GetFloatConstant(1.0f);
    op = node->GetOp() == IncDecExpr::Op::Inc ? spv::OpFAdd : spv::OpFSub;
  }
  uint32_t result = AppendCode(op, typeId, {value, one});
  AppendCode(spv::Op::OpStore, {ptr, result});
  return node->returnOrigValue() ? value : result;
}

Result CodeGenSPIRV::Visit(WhileStatement* whileStmt) {
  uint32_t topOfLoop = NextId();
  uint32_t condition = NextId();
  uint32_t loopBody = NextId();
  uint32_t exitLoop = NextId();
  AppendCode(spv::Op::OpBranch, {topOfLoop});
  AppendCode(spv::Op::OpLabel, {topOfLoop});
  AppendCode(spv::Op::OpLoopMerge, {exitLoop, loopBody, 0});
  AppendCode(spv::Op::OpBranch, {condition});
  AppendCode(spv::Op::OpLabel, {condition});
  uint32_t cond = GenerateSPIRV(whileStmt->GetCond());
  AppendCode(spv::Op::OpBranchConditional, {cond, loopBody, exitLoop});
  AppendCode(spv::Op::OpLabel, {loopBody});
  GenerateSPIRV(whileStmt->GetBody());
  AppendCode(spv::Op::OpBranch, {topOfLoop});
  AppendCode(spv::Op::OpLabel, {exitLoop});
  return 0u;
}

void CodeGenSPIRV::ICE(ASTNode* node) { NOTIMPLEMENTED(); }

};  // namespace Toucan
