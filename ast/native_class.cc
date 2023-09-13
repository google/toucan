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

#include "type.h"

namespace Toucan {

ClassType* NativeClass::BindGroup;
ClassType* NativeClass::Buffer;
ClassType* NativeClass::CommandBuffer;
ClassType* NativeClass::CommandEncoder;
ClassType* NativeClass::ComputePassEncoder;
ClassType* NativeClass::ComputePipeline;
ClassType* NativeClass::Device;
ClassType* NativeClass::Event;
ClassType* NativeClass::ImageDecoder;
ClassType* NativeClass::Math;
ClassType* NativeClass::Queue;
ClassType* NativeClass::RenderPassEncoder;
ClassType* NativeClass::RenderPipeline;
ClassType* NativeClass::Sampler;
ClassType* NativeClass::SwapChain;
ClassType* NativeClass::System;
ClassType* NativeClass::Texture1D;
ClassType* NativeClass::Texture1DView;
ClassType* NativeClass::Texture2D;
ClassType* NativeClass::Texture2DView;
ClassType* NativeClass::Texture2DArrayView;
ClassType* NativeClass::Texture3D;
ClassType* NativeClass::Texture3DView;
ClassType* NativeClass::TextureCubeArrayView;
ClassType* NativeClass::TextureCubeView;
ClassType* NativeClass::Window;

}  // namespace Toucan
