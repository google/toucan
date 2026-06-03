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

#ifndef NATIVE_CLASS_H_
#define NATIVE_CLASS_H_

#include <string>

// TODO: autogenerate this from api.t.

namespace Toucan {

enum class NativeClass {
  None,
  BindGroup,
  Buffer,
  ColorOutput,
  CommandBuffer,
  CommandEncoder,
  ComputePass,
  ComputePipeline,
  DepthStencilOutput,
  Device,
  Event,
  Image,
  Math,
  Queue,
  RenderPass,
  RenderPipeline,
  SampleableTexture1D,
  SampleableTexture2D,
  SampleableTexture2DArray,
  SampleableTexture3D,
  SampleableTextureCube,
  Sampler,
  SwapChain,
  System,
  Texture1D,
  Texture2D,
  Texture2DArray,
  Texture3D,
  TextureCube,
  VertexInput,
  Window,
};

NativeClass FindNativeClass(std::string className);

};  // namespace Toucan
#endif
