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

#ifndef _APIINTERNAL_H
#define _APIINTERNAL_H

#include <webgpu/webgpu_cpp.h>

namespace Toucan {

struct Device {
  Device(wgpu::Device d) : device(d) {}
  wgpu::Device device;
};

struct SwapChain {
#if TARGET_OS_IS_WASM
  SwapChain(wgpu::SwapChain sc, wgpu::Surface s, wgpu::Device d, wgpu::Extent3D e, wgpu::TextureFormat f, void* p)
      : swapChain(sc), surface(s), device(d), extent(e), format(f), pool(p) {}
  wgpu::SwapChain     swapChain;
#else
  SwapChain(wgpu::Surface s, wgpu::Device d, wgpu::Extent3D e, wgpu::TextureFormat f, void* p)
      : surface(s), device(d), extent(e), format(f), pool(p) {}
#endif
  wgpu::Surface       surface;
  wgpu::Device        device;
  wgpu::Extent3D      extent;
  wgpu::TextureFormat format;
  void*               pool;
};

wgpu::TextureFormat GetPreferredSwapChainFormat();
wgpu::TextureFormat ToDawnTextureFormat(Type* type);
wgpu::Device CreateDawnDevice(wgpu::BackendType type, const wgpu::DeviceDescriptor* desc);

}  // namespace Toucan
#endif  // _APIINTERNAL_H
