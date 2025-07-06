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
#include <ast/shader_prep_pass.h>

#define NOTIMPLEMENTED() assert(false)

namespace {
const int WordCountShift = 16;
}

namespace Toucan {

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
  return isSampleableTexture1D(classType) || isSampleableTexture2D(classType) ||
         isSampleableTexture3D(classType) || isSampleableTexture2DArray(classType) ||
         isSampleableTextureCube(classType);
}

bool isMath(ClassType* classType) { return classType == NativeClass::Math; }

bool isSystem(ClassType* classType) { return classType == NativeClass::System; }

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

spv::ExecutionModel toExecutionModel(int modifiers) {
  if (modifiers & Method::Modifier::Vertex) {
    return spv::ExecutionModelVertex;
  } else if (modifiers & Method::Modifier::Fragment) {
    return spv::ExecutionModelFragment;
  } else if (modifiers & Method::Modifier::Compute) {
    return spv::ExecutionModelGLCompute;
  }
  assert(false);
  return spv::ExecutionModelFragment;
}

uint32_t CodeGenSPIRV::GetStorageClass(Type* type) {
  if (type->IsPtr()) { type = static_cast<PtrType*>(type)->GetBaseType(); }
  int qualifiers;
  type = type->GetUnqualifiedType(&qualifiers);
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
  }
  return spv::StorageClassFunction;
}

void CodeGenSPIRV::DeclareBuiltInVars(const VarVector& builtInVars, Code* interface) {
  for (auto var : builtInVars) {
    uint32_t storageClass =
        var->type->IsWriteable() ? spv::StorageClassOutput : spv::StorageClassInput;
    uint32_t typeId = ConvertPointerToType(var->type, storageClass);
    uint32_t ptrId = AppendDecl(spv::Op::OpVariable, typeId, {storageClass});
    uint32_t builtinId = builtinNameToID(var->name);
    Append(spv::OpDecorate, {ptrId, spv::DecorationBuiltIn, builtinId}, &annotations_);
    vars_[var.get()] = ptrId;
    interface->push_back(ptrId);
  }
}

uint32_t CodeGenSPIRV::GetSampledImageType(Type* type) {
  auto t = sampledImageTypes_.find(type);
  if (t != sampledImageTypes_.end()) { return t->second; }
  uint32_t sampledImageType = AppendTypeDecl(spv::OpTypeSampledImage, {ConvertType(type)});
  sampledImageTypes_[type] = sampledImageType;
  return sampledImageType;
}

void CodeGenSPIRV::DeclareInterfaceVars(const VarVector& vars,
                                        uint32_t         storageClass,
                                        Code*            interface) {
  for (uint32_t i = 0; i < vars.size(); ++i) {
    Type*    type = vars[i]->type;
    uint32_t ptrToType = ConvertPointerToType(type, storageClass);
    uint32_t ptrId = AppendDecl(spv::Op::OpVariable, ptrToType, {storageClass});
    vars_[vars[i].get()] = ptrId;
    interface->push_back(ptrId);
    Append(spv::OpDecorate, {ptrId, spv::DecorationLocation, i}, &annotations_);
    if (type->IsInteger()) {
      if (((storageClass == spv::StorageClassInput && (methodModifiers_ & Method::Modifier::Fragment)) ||
           (storageClass == spv::StorageClassOutput && (methodModifiers_ & Method::Modifier::Vertex)))) {
        Append(spv::OpDecorate, {ptrId, spv::DecorationFlat}, &annotations_);
      }
    }
  }
}

void CodeGenSPIRV::DeclareBindGroupVars(const BindGroupList& bindGroups) {
  uint32_t group = 0;
  for (auto& bindGroup : bindGroups) {
    uint32_t binding = 0;
    for (auto& var : bindGroup) {
      uint32_t varId = DeclareVar(var.get());
      Append(spv::OpDecorate, {varId, spv::DecorationDescriptorSet, group}, &annotations_);
      Append(spv::OpDecorate, {varId, spv::DecorationBinding, binding}, &annotations_);
      int qualifiers;
      var->type->GetUnqualifiedType(&qualifiers);
      if (qualifiers & Type::Qualifier::Coherent) {
        Append(spv::OpDecorate, {varId, spv::DecorationCoherent}, &annotations_);
      }
      binding++;
    }
    group++;
  }
  bindGroups_ = bindGroups;
}

void CodeGenSPIRV::Run(Method* entryPoint) {
  glslStd450Import_ = NextId();
  uint32_t functionId = NextId();
  methodModifiers_ = entryPoint->modifiers;
  assert(entryPoint->formalArgList.size() > 0);

  NodeVector     nodes;
  ShaderPrepPass shaderPrepPass(&nodes, types_);
  entryPoint = shaderPrepPass.Run(entryPoint);

  Code interface;
  DeclareInterfaceVars(shaderPrepPass.GetInputs(), spv::StorageClassInput, &interface);
  DeclareInterfaceVars(shaderPrepPass.GetOutputs(), spv::StorageClassOutput, &interface);
  DeclareBuiltInVars(shaderPrepPass.GetBuiltInVars(), &interface);
  DeclareBindGroupVars(shaderPrepPass.GetBindGroups());
  GenCodeForMethod(entryPoint, functionId);
  while (!pendingMethods_.empty()) {
    Method* m = pendingMethods_.front();
    pendingMethods_.pop_front();
    GenCodeForMethod(m, functions_[m]);
  }

  header_.push_back(spv::MagicNumber);
  header_.push_back(0x00010300);
  header_.push_back(0);        // Generator
  header_.push_back(nextID_);  // Bound
  header_.push_back(0);        // Schema
  Append(spv::OpCapability, {spv::CapabilityMatrix}, &header_);
  Append(spv::OpCapability, {spv::CapabilityShader}, &header_);
  Append(spv::OpCapability, {spv::CapabilityImageQuery}, &header_);
  Append(spv::OpCapability, {spv::CapabilitySampled1D}, &header_);
  Append(spv::OpCapability, {spv::CapabilityImage1D}, &header_);
  Code importName;
  AppendString("GLSL.std.450", &importName);
  header_.push_back(spv::OpExtInstImport | ((2 + importName.size()) << WordCountShift));
  header_.push_back(glslStd450Import_);
  header_.insert(header_.end(), importName.begin(), importName.end());
  Append(spv::OpMemoryModel, {spv::AddressingModelLogical, spv::MemoryModelGLSL450}, &header_);
  uint32_t executionModel = toExecutionModel(methodModifiers_);
  AppendEntryPoint(executionModel, functionId, "main", interface);
  if (methodModifiers_ & Method::Modifier::Fragment) {
    Append(spv::OpExecutionMode, {functionId, spv::ExecutionModeOriginUpperLeft}, &header_);
  } else if (methodModifiers_ & Method::Modifier::Compute) {
    auto ws = entryPoint->workgroupSize;
    Append(spv::OpExecutionMode, {functionId, spv::ExecutionModeLocalSize, ws[0], ws[1], ws[2]},
           &header_);
  }
}

uint32_t CodeGenSPIRV::GenerateSPIRV(ASTNode* node) { return std::get<uint32_t>(node->Accept(this)); }

Result CodeGenSPIRV::Visit(ArrayAccess* node) {
  uint32_t resultType = ConvertType(node->GetType(types_));
  uint32_t base = GenerateSPIRV(node->GetExpr());
  uint32_t index = GenerateSPIRV(node->GetIndex());
  return AppendCode(spv::Op::OpAccessChain, resultType, {base, index});
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

uint32_t CodeGenSPIRV::AppendCodeFromExprList(uint32_t opCode, uint32_t resultType, ExprList* exprList) {
  Code args;
  for (auto& i : exprList->Get()) {
    args.push_back(GenerateSPIRV(i));
  }
  return AppendCode(opCode, resultType, args);
}

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
  assert(!var->type->IsPtr());
  uint32_t storageClass = GetStorageClass(var->type);
  uint32_t varType = ConvertPointerToType(var->type);
  return vars_[var] = AppendCode(spv::Op::OpVariable, varType, {storageClass});
}

uint32_t CodeGenSPIRV::AppendImageDecl(uint32_t        dim,
                                       bool            array,
                                       int             qualifiers,
                                       const TypeList& templateArgs) {
  assert(templateArgs.size() >= 1);
  uint32_t sampledType = ConvertType(templateArgs[0]);
  uint32_t depth = 0;  // not depth
  uint32_t arrayed = array ? 1 : 0;
  uint32_t ms = 0;       // not multisampled
  uint32_t sampled = 1;  // sampled
  uint32_t format = spv::ImageFormatUnknown;
  return AppendTypeDecl(spv::Op::OpTypeImage,
                        {sampledType, dim, depth, arrayed, ms, sampled, format});
}

uint32_t CodeGenSPIRV::ConvertType(Type* type) {
  int qualifiers;
  type = type->GetUnqualifiedType(&qualifiers);
  bool isInterfaceBlock = qualifiers & (Type::Qualifier::Uniform | Type::Qualifier::Storage);
  if (spirvTypes_[type] != 0) { return spirvTypes_[type]; }
  uint32_t resultId;
  if (type->IsPtr()) {
    Type* baseType = static_cast<PtrType*>(type)->GetBaseType();
    return ConvertPointerToType(baseType);
  } else if (type->IsArray()) {
    ArrayType* a = static_cast<ArrayType*>(type);
    uint32_t   elementType = ConvertType(a->GetElementType());
    if (a->GetNumElements() == 0) {
      resultId = AppendTypeDecl(spv::Op::OpTypeRuntimeArray, {elementType});
    } else {
      uint32_t intType = ConvertType(types_->GetInt());
      uint32_t value = a->GetNumElements();
      uint32_t numElementsId = GetIntConstant(value);
      resultId = AppendTypeDecl(spv::Op::OpTypeArray, {elementType, numElementsId});
    }
    Append(spv::OpDecorate,
           {resultId, spv::DecorationArrayStride, (uint32_t)a->GetElementSizeInBytes()},
           &annotations_);
  } else if (type->IsClass()) {
    ClassType* classType = static_cast<ClassType*>(type);
    if (isSampler(classType)) {
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
      Code args;
      for (auto& field : classType->GetFields()) {
        args.push_back(ConvertType(field->type));
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
    uint32_t    componentType = ConvertType(v->GetElementType());
    resultId = AppendTypeDecl(spv::Op::OpTypeVector, {componentType, v->GetNumElements()});
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
  return spirvTypes_[type] = resultId;
}

uint32_t CodeGenSPIRV::GetFunctionType(const Code& signature) {
  if (spirvFunctionTypes_[signature] != 0) { return spirvFunctionTypes_[signature]; }
  uint32_t resultId = AppendTypeDecl(spv::Op::OpTypeFunction, signature);
  return spirvFunctionTypes_[signature] = resultId;
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

  // Create function-local storage for any non-pointer arguments.
  for (auto arg : method->formalArgList) {
    if (!arg->type->IsPtr()) { DeclareVar(arg.get()); }
  }
  for (auto var : method->stmts->GetVars()) {
    DeclareVar(var.get());
  }
  auto fp = fps.begin();
  // Store non-pointer arguments into function-local variables.
  for (auto arg : method->formalArgList) {
    if (!arg->type->IsPtr()) {
      AppendCode(spv::Op::OpStore, {vars_[arg.get()], *fp++});
    } else {
      vars_[arg.get()] = *fp++;
    }
    assert(vars_[arg.get()] != 0);
  }
  GenerateSPIRV(method->stmts);

  AppendCode(spv::Op::OpFunctionEnd, {});
}

uint32_t CodeGenSPIRV::ConvertPointerToType(Type* type) {
  return ConvertPointerToType(type, GetStorageClass(type));
}

uint32_t CodeGenSPIRV::ConvertPointerToType(Type* type, uint32_t storageClass) {
  PtrTypeKey key(type->GetUnqualifiedType(), storageClass);
  if (spirvPtrTypes_[key] != 0) { return spirvPtrTypes_[key]; }
  uint32_t typeId = ConvertType(type);
  uint32_t resultId = AppendTypeDecl(spv::Op::OpTypePointer, {storageClass, typeId});
  return spirvPtrTypes_[key] = resultId;
}

uint32_t CodeGenSPIRV::CreateVectorSplat(uint32_t value, VectorType* type) {
  uint32_t resultType = ConvertType(type);
  Code     args;
  for (int i = 0; i < type->GetNumElements(); ++i) {
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
  return AppendCode(opCode, typeId, {lhs, rhs});
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
  return AppendCode(opCode, typeId, {rhs});
}

Result CodeGenSPIRV::Visit(BoolConstant* expr) { return GetBoolConstant(expr->GetValue()); }

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
  } else if (dstType->IsFloat() && srcType->IsUInt()) {
    return AppendCode(spv::Op::OpConvertUToF, resultType, {valueId});
  } else if (dstType->IsUInt() && srcType->IsFloat()) {
    return AppendCode(spv::Op::OpConvertFToU, resultType, {valueId});
  } else if (dstType->IsInt() && srcType->IsUInt()) {
    return valueId;
  } else if (dstType->IsUInt() && srcType->IsInt()) {
    return valueId;
  } else if (dstType->IsVector() && srcType->IsVector()) {
    VectorType* srcVectorType = static_cast<VectorType*>(srcType);
    VectorType* dstVectorType = static_cast<VectorType*>(dstType);
    assert(srcVectorType->GetNumElements() == dstVectorType->GetNumElements());
    return CreateCast(srcVectorType->GetElementType(), dstVectorType->GetElementType(),
                      resultType, valueId);
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
  uint32_t valueId = GenerateSPIRV(expr->GetExpr());
  if (srcType == dstType || srcType->IsPtr() && dstType->IsPtr()) { return valueId; }
  uint32_t resultType = ConvertType(dstType);
  return CreateCast(srcType, dstType, resultType, valueId);
}

Result CodeGenSPIRV::Visit(Initializer* node) {
  auto     args = node->GetArgList()->Get();
  Code     resultArgs;
  uint32_t resultType = ConvertType(node->GetType());
  for (auto arg : args) {
    resultArgs.push_back(GenerateSPIRV(arg));
  }
  return AppendCode(spv::Op::OpCompositeConstruct, resultType, {resultArgs});
}

Result CodeGenSPIRV::Visit(ExprWithStmt* node) {
  auto resultId = node->GetExpr() ? GenerateSPIRV(node->GetExpr()) : 0u;
  GenerateSPIRV(node->GetStmt());
  return resultId;
}

Result CodeGenSPIRV::Visit(SmartToRawPtr* node) { return GenerateSPIRV(node->GetExpr()); }

Result CodeGenSPIRV::Visit(DestroyStmt* node) { return 0u; }

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

uint32_t CodeGenSPIRV::GetConstant(Type* type, uint32_t value) {
  return AppendDecl(spv::Op::OpConstant, ConvertType(type), {value});
}

uint32_t CodeGenSPIRV::GetIntConstant(int32_t value) {
  if (intConstants_[value]) { return intConstants_[value]; }
  return intConstants_[value] = GetConstant(types_->GetInt(), value);
}

uint32_t CodeGenSPIRV::GetUIntConstant(uint32_t value) {
  if (uintConstants_[value]) { return uintConstants_[value]; }
  return uintConstants_[value] = GetConstant(types_->GetUInt(), value);
}

uint32_t CodeGenSPIRV::GetFloatConstant(float value) {
  if (floatConstants_[value]) { return floatConstants_[value]; }
  return floatConstants_[value] = GetConstant(types_->GetFloat(), *reinterpret_cast<uint32_t*>(&value));
}

uint32_t CodeGenSPIRV::GetBoolConstant(bool value) {
  int index = value ? 1 : 0;
  if (boolConstants_[index]) { return boolConstants_[index]; }
  uint32_t resultType = ConvertType(types_->GetBool());
  uint32_t op = value ? spv::Op::OpConstantTrue : spv::Op::OpConstantFalse;
  return boolConstants_[index] = AppendDecl(op, resultType, {});
}

uint32_t CodeGenSPIRV::GetZeroConstant(Type* type) {
  if (type->IsFloatingPoint()) {
    return GetFloatConstant(0.0);
  } else if (type->IsInt()) {
    return GetIntConstant(0);
  } else if (type->IsUInt()) {
    return GetUIntConstant(0u);
  } else if (type->IsBool()) {
    return GetBoolConstant(false);
  } else {
    assert(false);
    return 0;
  }
}

Result CodeGenSPIRV::Visit(FieldAccess* expr) {
  uint32_t base = GenerateSPIRV(expr->GetExpr());
  Field*   field = expr->GetField();

  uint32_t resultType = ConvertType(expr->GetType(types_));
  uint32_t index = GetIntConstant(field->index);
  return AppendCode(spv::Op::OpAccessChain, resultType, {base, index});
}

Result CodeGenSPIRV::Visit(FloatConstant* expr) {
  uint32_t resultType = ConvertType(expr->GetType(types_));
  float    value = expr->GetValue();
  return GetFloatConstant(expr->GetValue());
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
  AppendCode(spv::Op::OpSelectionMerge, {exitLabel, 0});
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
  return GetIntConstant(expr->GetValue());
}

Result CodeGenSPIRV::Visit(UIntConstant* expr) {
  return GetUIntConstant(expr->GetValue());
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
  Method*                   method = expr->GetMethod();
  const std::vector<Expr*>& args = expr->GetArgList()->Get();
  if (isTextureView(method->classType)) {
    if (method->name == "Sample") {
      uint32_t resultType = ConvertType(expr->GetType(types_));
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
    } else if (method->name == "Load") {
      uint32_t resultType = ConvertType(expr->GetType(types_));
      Type*    textureType = static_cast<PtrType*>(args[0]->GetType(types_))->GetBaseType();
      uint32_t texture = GenerateSPIRV(args[0]);
      texture = AppendCode(spv::Op::OpLoad, ConvertType(textureType), {texture});
      uint32_t coord = GenerateSPIRV(args[1]);
      uint32_t level;
      if (isSampleableTexture2DArray(method->classType)) {
        uint32_t layer = GenerateSPIRV(args[2]);
        uint32_t int3 = ConvertType(types_->GetVector(types_->GetInt(), 3));
        uint32_t intType = ConvertType(types_->GetInt());
        uint32_t x = AppendCode(spv::Op::OpCompositeExtract, intType, {coord, 0});
        uint32_t y = AppendCode(spv::Op::OpCompositeExtract, intType, {coord, 1});
        coord = AppendCode(spv::Op::OpCompositeConstruct, int3, {x, y, layer});
        level = GenerateSPIRV(args[3]);
      } else {
        level = GenerateSPIRV(args[2]);
      }
      uint32_t mask = spv::ImageOperandsLodMask;
      return AppendCode(spv::Op::OpImageFetch, resultType, {texture, coord, mask, level});
    } else if (method->name == "GetSize") {
      uint32_t resultType = ConvertType(expr->GetType(types_));
      Type*    textureType = static_cast<PtrType*>(args[0]->GetType(types_))->GetBaseType();
      uint32_t texture = GenerateSPIRV(args[0]);
      texture = AppendCode(spv::Op::OpLoad, ConvertType(textureType), {texture});
      return AppendCode(spv::Op::OpImageQuerySizeLod, resultType, {texture, GetIntConstant(0)});
    }
  } else if (isMath(method->classType)) {
    uint32_t resultType = ConvertType(expr->GetType(types_));
    ExprList* argList = expr->GetArgList();
    if (method->name == "all") {
      return AppendCodeFromExprList(spv::Op::OpAll, resultType, argList);
    } else if (method->name == "any") {
      return AppendCodeFromExprList(spv::Op::OpAny, resultType, argList);
    } else if (method->name == "sin") {
      return AppendExtInst(GLSLstd450Sin, resultType, argList);
    } else if (method->name == "cos") {
      return AppendExtInst(GLSLstd450Cos, resultType, argList);
    } else if (method->name == "tan") {
      return AppendExtInst(GLSLstd450Tan, resultType, argList);
    } else if (method->name == "dot") {
      return AppendCodeFromExprList(spv::Op::OpDot, resultType, argList);
    } else if (method->name == "cross") {
      return AppendExtInst(GLSLstd450Cross, resultType, argList);
    } else if (method->name == "fabs") {
      return AppendExtInst(GLSLstd450FAbs, resultType, argList);
    } else if (method->name == "clz") {
      return AppendExtInst(GLSLstd450FindSMsb, resultType, argList);
    } else if (method->name == "floor") {
      return AppendExtInst(GLSLstd450Floor, resultType, argList);
    } else if (method->name == "ceil") {
      return AppendExtInst(GLSLstd450Ceil, resultType, argList);
    } else if (method->name == "length") {
      return AppendExtInst(GLSLstd450Length, resultType, argList);
    } else if (method->name == "min") {
      return AppendExtInst(GLSLstd450FMin, resultType, argList);
    } else if (method->name == "max") {
      return AppendExtInst(GLSLstd450FMax, resultType, argList);
    } else if (method->name == "pow") {
      return AppendExtInst(GLSLstd450Pow, resultType, argList);
    } else if (method->name == "reflect") {
      return AppendExtInst(GLSLstd450Reflect, resultType, argList);
    } else if (method->name == "refract") {
      return AppendExtInst(GLSLstd450Refract, resultType, argList);
    } else if (method->name == "normalize") {
      return AppendExtInst(GLSLstd450Normalize, resultType, argList);
    } else if (method->name == "inverse") {
      return AppendExtInst(GLSLstd450MatrixInverse, resultType, argList);
    } else if (method->name == "transpose") {
      return AppendCodeFromExprList(spv::Op::OpTranspose, resultType, argList);
    }
  } else if (isSystem(method->classType)) {
    if (method->name == "StorageBarrier") {
      Code resultArgs;
      resultArgs.push_back(GetIntConstant(spv::ScopeWorkgroup));
      resultArgs.push_back(GetIntConstant(spv::ScopeDevice));
      resultArgs.push_back(GetIntConstant(spv::MemorySemanticsUniformMemoryMask |
                                          spv::MemorySemanticsAcquireReleaseMask));
      AppendCode(spv::Op::OpControlBarrier, resultArgs);
      return {};  // FIXME: handle void method returns
    }
  }
  uint32_t functionId = functions_[method];
  if (functionId == 0) {
    functionId = NextId();
    functions_[method] = functionId;
    pendingMethods_.push_back(method);
  }
  Code resultArgs;
  resultArgs.push_back(functionId);
  for (auto& i : args) {
    resultArgs.push_back(GenerateSPIRV(i));
  }
  uint32_t resultType = ConvertType(expr->GetType(types_));
  return AppendCode(spv::Op::OpFunctionCall, resultType, resultArgs);
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
  return AppendCode(spv::Op::OpCompositeExtract, resultType, {compositeId, index});
}

Result CodeGenSPIRV::Visit(InsertElementExpr* expr) {
  uint32_t resultType = ConvertType(expr->GetType(types_));
  uint32_t compositeId = GenerateSPIRV(expr->GetExpr());
  uint32_t newElementId = GenerateSPIRV(expr->newElement());
  uint32_t index = expr->GetIndex();
  return AppendCode(spv::Op::OpCompositeInsert, resultType, {newElementId, compositeId, index});
}

Result CodeGenSPIRV::Visit(VarExpr* expr) { return vars_[expr->GetVar()]; }

Result CodeGenSPIRV::Visit(LoadExpr* expr) {
  uint32_t exprId = GenerateSPIRV(expr->GetExpr());
  uint32_t resultType = ConvertType(expr->GetType(types_));
  return AppendCode(spv::Op::OpLoad, resultType, {exprId});
}

Result CodeGenSPIRV::Visit(ZeroInitStmt* stmt) {
  // All variables are zero-initialized already
  return 0u;
}

Result CodeGenSPIRV::Visit(StoreStmt* stmt) {
  uint32_t objectId = GenerateSPIRV(stmt->GetRHS());
  uint32_t pointerId = GenerateSPIRV(stmt->GetLHS());
  AppendCode(spv::Op::OpStore, {pointerId, objectId});
  return 0u;
}

Result CodeGenSPIRV::Visit(SwizzleExpr* node) {
  uint32_t resultType = ConvertType(node->GetType(types_));
  uint32_t value = GenerateSPIRV(node->GetExpr());
  Code args;
  args.push_back(value);
  args.push_back(value);
  for (uint32_t index : node->GetIndices()) {
    args.push_back(index);
  }
  return AppendCode(spv::Op::OpVectorShuffle, resultType, args);
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
