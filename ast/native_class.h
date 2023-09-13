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
#endif

// TODO: autogenerate this from api.t.

namespace Toucan {

class ClassType;

class NativeClass {
 public:
  static ClassType* BindGroup;
  static ClassType* Buffer;
  static ClassType* CommandBuffer;
  static ClassType* CommandEncoder;
  static ClassType* ComputePassEncoder;
  static ClassType* ComputePipeline;
  static ClassType* Device;
  static ClassType* Event;
  static ClassType* ImageDecoder;
  static ClassType* Math;
  static ClassType* Queue;
  static ClassType* RenderPassEncoder;
  static ClassType* RenderPipeline;
  static ClassType* Sampler;
  static ClassType* SwapChain;
  static ClassType* System;
  static ClassType* Texture1D;
  static ClassType* Texture1DView;
  static ClassType* Texture2D;
  static ClassType* Texture2DView;
  static ClassType* Texture2DArrayView;
  static ClassType* Texture3D;
  static ClassType* Texture3DView;
  static ClassType* TextureCubeArrayView;
  static ClassType* TextureCubeView;
  static ClassType* Window;
};

};  // namespace Toucan
