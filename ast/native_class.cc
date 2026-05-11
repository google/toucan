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

#include "native_class.h"

#include "ast.h"
#include "type.h"

namespace Toucan {

ClassType* NativeClass::BindGroup;
ClassType* NativeClass::Buffer;
ClassType* NativeClass::ColorOutput;
ClassType* NativeClass::CommandBuffer;
ClassType* NativeClass::CommandEncoder;
ClassType* NativeClass::ComputePass;
ClassType* NativeClass::ComputePipeline;
ClassType* NativeClass::DepthStencilOutput;
ClassType* NativeClass::Device;
ClassType* NativeClass::Event;
ClassType* NativeClass::Image;
ClassType* NativeClass::Math;
ClassType* NativeClass::Queue;
ClassType* NativeClass::RenderPass;
ClassType* NativeClass::RenderPipeline;
ClassType* NativeClass::SampleableTexture1D;
ClassType* NativeClass::SampleableTexture2D;
ClassType* NativeClass::SampleableTexture2DArray;
ClassType* NativeClass::SampleableTexture3D;
ClassType* NativeClass::SampleableTextureCube;
ClassType* NativeClass::Sampler;
ClassType* NativeClass::SwapChain;
ClassType* NativeClass::System;
ClassType* NativeClass::Texture1D;
ClassType* NativeClass::Texture2D;
ClassType* NativeClass::Texture2DArray;
ClassType* NativeClass::Texture3D;
ClassType* NativeClass::TextureCube;
ClassType* NativeClass::VertexInput;
ClassType* NativeClass::Window;

void InitNativeClasses(Scope* scope) {
  auto findClassType = [scope](const char* id) -> ClassType* {
    return static_cast<ClassType*>(scope->FindType(id));
  };
  NativeClass::BindGroup = findClassType("BindGroup");
  NativeClass::Buffer = findClassType("Buffer");
  NativeClass::ColorOutput = findClassType("ColorOutput");
  NativeClass::CommandBuffer = findClassType("CommandBuffer");
  NativeClass::CommandEncoder = findClassType("CommandEncoder");
  NativeClass::ComputePass = findClassType("ComputePass");
  NativeClass::ComputePipeline = findClassType("ComputePipeline");
  NativeClass::DepthStencilOutput = findClassType("DepthStencilOutput");
  NativeClass::Device = findClassType("Device");
  NativeClass::Event = findClassType("Event");
  NativeClass::Image = findClassType("Image");
  NativeClass::Math = findClassType("Math");
  NativeClass::Queue = findClassType("Queue");
  NativeClass::RenderPass = findClassType("RenderPass");
  NativeClass::RenderPipeline = findClassType("RenderPipeline");
  NativeClass::SampleableTexture1D = findClassType("SampleableTexture1D");
  NativeClass::SampleableTexture2D = findClassType("SampleableTexture2D");
  NativeClass::SampleableTexture2DArray = findClassType("SampleableTexture2DArray");
  NativeClass::SampleableTexture3D = findClassType("SampleableTexture3D");
  NativeClass::SampleableTextureCube = findClassType("SampleableTextureCube");
  NativeClass::Sampler = findClassType("Sampler");
  NativeClass::SwapChain = findClassType("SwapChain");
  NativeClass::System = findClassType("System");
  NativeClass::Texture1D = findClassType("Texture1D");
  NativeClass::Texture2D = findClassType("Texture2D");
  NativeClass::Texture2DArray = findClassType("Texture2DArray");
  NativeClass::Texture3D = findClassType("Texture3D");
  NativeClass::TextureCube = findClassType("TextureCube");
  NativeClass::VertexInput = findClassType("VertexInput");
  NativeClass::Window = findClassType("Window");
}

}  // namespace Toucan
