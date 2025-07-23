// Copyright 2025 The Toucan Authors
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

#include "api_validator.h"

#include <stdarg.h>
#include <filesystem>

#include "native_class.h"
#include "shader_validation_pass.h"

namespace Toucan {

namespace {

bool IsValidVertexAttributeType(Type* type) {
  if (type->IsVector()) {
    return IsValidVertexAttributeType(static_cast<VectorType*>(type)->GetElementType());
  }

  return type->IsFloat() || type->IsInt() || type->IsUInt();
}

bool IsValidRenderPipelineField(Type* type) {
  if (!type->IsStrongPtr()) return false;

  type = static_cast<StrongPtrType*>(type)->GetBaseType();
  int qualifiers;
  type = type->GetUnqualifiedType(&qualifiers);
  if (!type->IsClass()) return false;
  auto classType = static_cast<ClassType*>(type);
  auto templ = classType->GetTemplate();
  if (templ == NativeClass::VertexInput) return true;
  if (templ == NativeClass::Buffer) return qualifiers == Type::Qualifier::Index;
  if (templ == NativeClass::ColorAttachment) return true;
  if (templ == NativeClass::DepthStencilAttachment) return true;  // Ibid.
  if (templ == NativeClass::BindGroup) return true;
  return false;
}

bool IsValidComputePipelineField(Type* type) {
  if (!type->IsStrongPtr()) return false;

  type = static_cast<StrongPtrType*>(type)->GetBaseType();
  int qualifiers;
  type = type->GetUnqualifiedType(&qualifiers);
  if (!type->IsClass()) return false;
  auto classType = static_cast<ClassType*>(type);
  return classType->GetTemplate() == NativeClass::BindGroup;
}

bool IsValidBindGroupFieldType(Type* type) {
  if (!type->IsStrongPtr()) return false;
  type = static_cast<StrongPtrType*>(type)->GetBaseType();
  int qualifiers;
  type = type->GetUnqualifiedType(&qualifiers);
  if (!type->IsClass()) return false;
  auto classType = static_cast<ClassType*>(type);
  if (classType == NativeClass::Sampler && qualifiers == 0) return true;

  auto templ = classType->GetTemplate();

  if (templ == NativeClass::Buffer &&
      (qualifiers & (Type::Qualifier::Uniform | Type::Qualifier::Storage))) {
    return true;
  }
  if (templ == NativeClass::SampleableTexture1D || templ == NativeClass::SampleableTexture2D ||
      templ == NativeClass::SampleableTexture2DArray || templ == NativeClass::SampleableTexture3D ||
      templ == NativeClass::SampleableTextureCube)
    return true;
  return false;
}

}  // namespace

APIValidator::APIValidator() {}

void APIValidator::ValidateType(Type* type, const FileLocation& fileLocation) {
  if (type->IsPtr()) type = static_cast<PtrType*>(type)->GetBaseType();
  int qualifiers;
  type = type->GetUnqualifiedType(&qualifiers);
  if (!type->IsClass() || !type->IsFullySpecified()) return;

  ScopedFileLocation scopedFile(&fileLocation_, fileLocation);

  auto classType = static_cast<ClassType*>(type);
  auto classTemplate = classType->GetTemplate();
  if (!classTemplate) return;

  if (classTemplate == NativeClass::Buffer) {
    ValidateBuffer(classType, qualifiers);
  } else if (classTemplate == NativeClass::BindGroup) {
    ValidateBindGroup(classType);
  } else if (classTemplate == NativeClass::RenderPipeline) {
    ValidateRenderPipeline(classType);
  } else if (classTemplate == NativeClass::RenderPass) {
    ValidateRenderPipelineFields(classType);
  } else if (classTemplate == NativeClass::ComputePipeline) {
    ValidateComputePipeline(classType);
  } else if (classTemplate == NativeClass::ComputePass) {
    ValidateComputePipelineFields(classType);
  }
}

void APIValidator::ValidateVertexAttributeType(ClassType* buffer, Type* type) {
  if (!IsValidVertexAttributeType(type)) {
    Error(buffer, "%s is not a valid vertex attribute type", type->ToString().c_str());
  }
}

void APIValidator::ValidateVertexBufferType(ClassType* buffer, Type* type) {
  if (!type->IsUnsizedArray()) {
    Error(buffer, "%s is not a runtime-sized array", type->ToString().c_str());
    return;
  }
  type = static_cast<ArrayType*>(type)->GetElementType();
  if (type->IsClass()) {
    for (const auto& field : static_cast<ClassType*>(type)->GetFields()) {
      ValidateVertexAttributeType(buffer, field->type);
    }
  } else {
    ValidateVertexAttributeType(buffer, type);
  }
}

void APIValidator::ValidateIndexBufferType(ClassType* buffer, Type* type) {
  if (!type->IsUnsizedArray()) {
    Error(buffer, "%s is not a runtime-sized array", type->ToString().c_str());
    return;
  }
  type = static_cast<ArrayType*>(type)->GetElementType();
  if (!(type->IsUInt() || type->IsUShort())) {
    Error(buffer, "%s is not a valid index buffer type; must be uint or ushort",
          type->ToString().c_str());
  }
}

void APIValidator::ValidateUniformDataType(ClassType* buffer, Type* type) {
  if (type->IsClass()) {
    auto classType = static_cast<ClassType*>(type);
    for (const auto& field : classType->GetFields()) {
      ValidateUniformDataType(buffer, field->type);
    }
  } else if (type->IsArrayLike()) {
    if (type->IsUnsizedArray()) {
      Error(buffer, "%s: runtime-sized arrays are prohibited in uniform buffers",
            type->ToString().c_str());
    }
    ValidateUniformDataType(buffer, static_cast<ArrayLikeType*>(type)->GetElementType());
  } else if (!type->IsFloat() && !type->IsInt() && !type->IsUInt()) {
    Error(buffer, "%s is not a valid uniform buffer type", type->ToString().c_str());
  }
}

void APIValidator::ValidateStorageDataType(ClassType* buffer, Type* type) {
  if (type->IsClass()) {
    auto classType = static_cast<ClassType*>(type);
    for (const auto& field : classType->GetFields()) {
      ValidateStorageDataType(buffer, field->type);
    }
  } else if (type->IsArrayLike()) {
    ValidateStorageDataType(buffer, static_cast<ArrayLikeType*>(type)->GetElementType());
  } else if (!type->IsFloat() && !type->IsInt() && !type->IsUInt()) {
    Error(buffer, "%s is not a valid storage buffer type", type->ToString().c_str());
  }
}

void APIValidator::ValidateBuffer(ClassType* buffer, int qualifiers) {
  struct QualifierInfo {
    Type::Qualifier qualifier;
    const char*     str;
  };
  constexpr std::array<QualifierInfo, 3> invalidBufferQualifiers = {
      Type::Qualifier::Sampleable,   "sampleable",  Type::Qualifier::Renderable, "renderable",
      Type::Qualifier::Unfilterable, "unfilterable"};
  int DeviceBufferQualifiers = Type::Qualifier::Vertex | Type::Qualifier::Index |
                               Type::Qualifier::Uniform | Type::Qualifier::Storage;

  auto type = buffer->GetTemplateArgs()[0];
  if (qualifiers & Type::Qualifier::Vertex) { ValidateVertexBufferType(buffer, type); }
  if (qualifiers & Type::Qualifier::Index) { ValidateIndexBufferType(buffer, type); }
  if (qualifiers & Type::Qualifier::Uniform) { ValidateUniformDataType(buffer, type); }
  if (qualifiers & Type::Qualifier::Storage) { ValidateStorageDataType(buffer, type); }
  if (qualifiers & DeviceBufferQualifiers) {
    if (qualifiers & (Type::Qualifier::HostReadable | Type::Qualifier::HostWriteable)) {
      Error(buffer, "buffer can not have both host and device qualifiers");
    }
  }
  for (auto q : invalidBufferQualifiers) {
    if (qualifiers & q.qualifier) { Error(buffer, "invalid buffer qualifier: %s", q.str); }
  }
}

void APIValidator::ValidateBindGroup(ClassType* bindGroup) {
  auto type = bindGroup->GetTemplateArgs()[0];
  if (!type->IsClass()) {
    Error(bindGroup, "bind group template argument must be of class type");
    return;
  }
  auto classType = static_cast<ClassType*>(bindGroup->GetTemplateArgs()[0]);
  for (const auto& field : classType->GetFields()) {
    if (!IsValidBindGroupFieldType(field->type)) {
      Error(bindGroup, "invalid bind group field type %s", field->type->ToString().c_str());
    }
  }
}

void APIValidator::ValidateRenderPipelineFields(ClassType* renderPipeline) {
  auto pipelineClass = static_cast<ClassType*>(renderPipeline->GetTemplateArgs()[0]);
  for (const auto& field : pipelineClass->GetFields()) {
    if (!IsValidRenderPipelineField(field->type)) {
      Error(renderPipeline, "%s is not a valid render pipeline field type",
            field->type->ToString().c_str());
    }
  }
}

void APIValidator::ValidateComputePipelineFields(ClassType* computePipeline) {
  auto pipelineClass = static_cast<ClassType*>(computePipeline->GetTemplateArgs()[0]);
  for (const auto& field : pipelineClass->GetFields()) {
    if (!IsValidComputePipelineField(field->type)) {
      Error(computePipeline, "%s is not a valid compute pipeline field type",
            field->type->ToString().c_str());
    }
  }
}

void APIValidator::ValidateRenderPipeline(ClassType* renderPipeline) {
  ValidateRenderPipelineFields(renderPipeline);
  Method* vertexShader = nullptr;
  Method* fragmentShader = nullptr;
  auto    pipelineClass = static_cast<ClassType*>(renderPipeline->GetTemplateArgs()[0]);
  for (ClassType* c = pipelineClass; c != nullptr && (!vertexShader || !fragmentShader);
       c = c->GetParent()) {
    for (auto& method : c->GetMethods()) {
      if (method->modifiers & Method::Modifier::Vertex) {
        if (!vertexShader) vertexShader = method.get();
      } else if (method->modifiers & Method::Modifier::Fragment) {
        if (!fragmentShader) fragmentShader = method.get();
      }
    }
  }
  if (!vertexShader) Error(renderPipeline, "no vertex shader found");
  if (!fragmentShader) Error(renderPipeline, "no fragment shader found");

  ShaderValidationPass shaderValidationPass;
  if (vertexShader) {
    shaderValidationPass.Run(vertexShader);
    numErrors_ += shaderValidationPass.GetNumErrors();
  }
  if (fragmentShader) {
    shaderValidationPass.Run(fragmentShader);
    numErrors_ += shaderValidationPass.GetNumErrors();
  }
}

void APIValidator::ValidateComputePipeline(ClassType* computePipeline) {
  ValidateComputePipelineFields(computePipeline);
  Method* shader = nullptr;
  auto    pipelineClass = static_cast<ClassType*>(computePipeline->GetTemplateArgs()[0]);
  for (ClassType* c = pipelineClass; c != nullptr && !shader; c = c->GetParent()) {
    for (auto& method : c->GetMethods()) {
      if (method->modifiers & Method::Modifier::Compute) { shader = method.get(); }
    }
  }
  if (!shader) {
    Error(computePipeline, "no compute shader found");
    return;
  }

  ShaderValidationPass shaderValidationPass;
  shaderValidationPass.Run(shader);
  numErrors_ += shaderValidationPass.GetNumErrors();
}

void APIValidator::Error(ClassType* instance, const char* fmt, ...) {
  std::string filename = fileLocation_.filename
                             ? std::filesystem::path(*fileLocation_.filename).filename().string()
                             : "";
  va_list     argp;
  va_start(argp, fmt);
  fprintf(stderr, "%s:%d:  while instantiating %s: ", filename.c_str(), fileLocation_.lineNum,
          instance->ToString().c_str());
  vfprintf(stderr, fmt, argp);
  fprintf(stderr, "\n");
  numErrors_++;
}

};  // namespace Toucan
