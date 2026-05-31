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

#include <string>
#include <unordered_map>

namespace Toucan {

namespace {

std::unordered_map<std::string, NativeClass> nativeClasses_;
std::unordered_map<NativeClass, std::string> nativeClassNames_;

void AddNativeClass(std::string name, NativeClass id) {
  nativeClasses_[name] = id;
  nativeClassNames_[id] = name;
}

void InitNativeClasses() {
  AddNativeClass("None", NativeClass::None);
  AddNativeClass("BindGroup", NativeClass::BindGroup);
  AddNativeClass("Buffer", NativeClass::Buffer);
  AddNativeClass("ColorOutput", NativeClass::ColorOutput);
  AddNativeClass("CommandBuffer", NativeClass::CommandBuffer);
  AddNativeClass("CommandEncoder", NativeClass::CommandEncoder);
  AddNativeClass("ComputePass", NativeClass::ComputePass);
  AddNativeClass("ComputePipeline", NativeClass::ComputePipeline);
  AddNativeClass("DepthStencilOutput", NativeClass::DepthStencilOutput);
  AddNativeClass("Device", NativeClass::Device);
  AddNativeClass("Event", NativeClass::Event);
  AddNativeClass("Image", NativeClass::Image);
  AddNativeClass("Math", NativeClass::Math);
  AddNativeClass("Queue", NativeClass::Queue);
  AddNativeClass("RenderPass", NativeClass::RenderPass);
  AddNativeClass("RenderPipeline", NativeClass::RenderPipeline);
  AddNativeClass("SampleableTexture1D", NativeClass::SampleableTexture1D);
  AddNativeClass("SampleableTexture2D", NativeClass::SampleableTexture2D);
  AddNativeClass("SampleableTexture2DArray", NativeClass::SampleableTexture2DArray);
  AddNativeClass("SampleableTexture3D", NativeClass::SampleableTexture3D);
  AddNativeClass("SampleableTextureCube", NativeClass::SampleableTextureCube);
  AddNativeClass("Sampler", NativeClass::Sampler);
  AddNativeClass("SwapChain", NativeClass::SwapChain);
  AddNativeClass("System", NativeClass::System);
  AddNativeClass("Texture1D", NativeClass::Texture1D);
  AddNativeClass("Texture2D", NativeClass::Texture2D);
  AddNativeClass("Texture2DArray", NativeClass::Texture2DArray);
  AddNativeClass("Texture3D", NativeClass::Texture3D);
  AddNativeClass("TextureCube", NativeClass::TextureCube);
  AddNativeClass("VertexInput", NativeClass::VertexInput);
  AddNativeClass("Window", NativeClass::Window);
}

}

NativeClass FindNativeClass(std::string className) {
  if (nativeClasses_.empty()) {
    InitNativeClasses();
  }
  return nativeClasses_[className];
}

std::string GetNativeClassName(NativeClass id) {
  if (nativeClasses_.empty()) {
    InitNativeClasses();
  }
  return nativeClassNames_[id];
}

}  // namespace Toucan
