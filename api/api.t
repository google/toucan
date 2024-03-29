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

enum PrimitiveTopology {
  PointList, LineList, LineStrip, TriangleList, TriangleStrip
}

native class CommandBuffer {
 ~CommandBuffer();
}

native class Queue {
 ~Queue();
  void Submit(CommandBuffer* commandBuffer);
}

native class Device {
  Device();
 ~Device();
  Queue* GetQueue();
}

native class Buffer<T> {
  Buffer(Device* device, uint size = 1);
 ~Buffer();
  void SetData(T^ data);
  readonly T^ MapRead();
  writeonly T^ MapWrite();
  readwrite T^ MapReadWrite();
  readonly uniform T^ MapReadUniform();
  writeonly storage T^ MapWriteStorage();
  readwrite storage T^ MapReadWriteStorage();
  void Unmap();
}

class DepthStencilState<T> {
  bool depthWriteEnabled = false;
//  CompareFunction depthCompare = CompareFunction::Always;
//  StencilFaceState stencilFront;
//  StencilFaceState stencilBack;
  uint stencilReadMask = 0xFFFFFFFF;
  uint stencilWriteMask = 0xFFFFFFFF;
  int depthBias = 0;
  float depthBiasSlopeScale = 0.0;
  float depthBiasClamp = 0.0;
}

native class RenderPipeline<T> {
  RenderPipeline(Device* device, DepthStencilState^ depthStencilState, PrimitiveTopology primitiveTopology);
 ~RenderPipeline();
}

native class ComputePipeline<T> {
  ComputePipeline(Device* device);
 ~ComputePipeline();
}

native class BindGroup {
  BindGroup(Device* device, void^ data);
 ~BindGroup();
}

native class RenderPassEncoder {
 ~RenderPassEncoder();
  void Draw(uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance);
  void DrawIndexed(uint indexCount, uint instanceCount, uint firstIndex, uint baseVertex, uint firstIntance);
  void SetBindGroup(uint groupIndex, BindGroup* bindGroup);
  void SetPipeline(RenderPipeline* pipeline);
  void SetIndexBuffer(index Buffer* buffer);
  void SetVertexBuffer(uint slot, vertex Buffer* buffer);
  void End();
}

native class ComputePassEncoder {
 ~ComputePassEncoder();
  void Dispatch(uint workgroupCountX, uint workgroupCountY, uint workgroupCountZ);
  void SetBindGroup(uint groupIndex, BindGroup* bindGroup);
  void SetPipeline(ComputePipeline* pipeline);
  void End();
}

enum AddressMode { Repeat, MirrorRepeat, ClampToEdge };

enum FilterMode { Nearest, Linear };

native class Sampler {
  Sampler(Device* device, AddressMode addressModeU, AddressMode addressModeV, AddressMode addressModeW, FilterMode magFilter, FilterMode minFilter, FilterMode mipmapFilter);
 ~Sampler();
}

native class Texture1DView<ST> {
 ~Texture1DView();
  ST<4> Sample(Sampler* sampler, float coord);
}

native class Texture2DView<ST> {
 ~Texture2DView();
  ST<4> Sample(Sampler* sampler, float<2> coords);
}

native class Texture2DArrayView<ST> {
 ~Texture2DArrayView();
  ST<4> Sample(Sampler* sampler, float<2> coords, uint layer);
}

native class Texture3DView<ST> {
 ~Texture3DView();
  ST<4> Sample(Sampler* sampler, float<3> coords);
}

native class TextureCubeView<ST> {
 ~TextureCubeView();
  ST<4> Sample(Sampler* sampler, float<3> coords);
}

native class TextureCubeArrayView<ST> {
 ~TextureCubeArrayView();
  ST<4> Sample(Sampler* sampler, float<3> coords, uint layer);
}

native class CommandEncoder;

native class Texture1D<PF> {
  Texture1D(Device* device, uint width);
 ~Texture1D();
  sampled Texture1DView<PF::SampledType>* CreateSampledView();
//  StorageTexture1DView<PF>* CreateStorageView();
  void CopyFromBuffer(CommandEncoder^ encoder, Buffer<PF::MemoryType[]>^ source, uint width);
}

native class Texture2D<PF> {
  Texture2D(Device* device, uint width, uint height, uint depth);
 ~Texture2D();
  sampled Texture2DView<PF::SampledType>* CreateSampledView();
  uint MinBufferWidth();
//  StorageTexture2DView<PF>* CreateStorageView();
  renderable Texture2DView<PF::SampledType>* CreateRenderableView();
  sampled TextureCubeArrayView<PF::SampledType>* CreateSampledCubeArrayView();
  sampled Texture2DArrayView<PF::SampledType>* CreateSampled2DArrayView();
  sampled TextureCubeView<PF::SampledType>* CreateSampledCubeView();
//  storage Texture2DArrayView<ST>* CreateStorage2DArrayView();
  void CopyFromBuffer(CommandEncoder^ encoder, Buffer<PF::MemoryType[]>^ source, uint width, uint height, uint depth, uint<3> origin);
}

native class Texture3D<PF> {
  Texture3D(Device* device, uint width, uint height, uint depth);
 ~Texture3D();
  uint MinBufferWidth();
  sampled Texture3DView<PF::SampledType>* CreateSampledView();
//  storage Texture3DView<PF>* CreateStorageView();
  void CopyFromBuffer(CommandEncoder^ encoder, Buffer<PF::MemoryType[]>^ source, uint width, uint height, uint depth);
}

native class CommandEncoder {
  CommandEncoder(Device* device);
 ~CommandEncoder();
  RenderPassEncoder* BeginRenderPass(renderable Texture2DView* colorAttachment, renderable Texture2DView* depthAttachment = null, float r = 0.0, float g = 0.0, float b = 0.0, float a = 0.0);
  ComputePassEncoder* BeginComputePass();
  CommandBuffer* Finish();
  void CopyBufferToBuffer(Buffer^ source, Buffer^ dest);
}

native class Window {
  Window(Device* device, int x, int y, uint width, uint height);
 ~Window();
}

native class SwapChain {
  SwapChain(Window^ window);
 ~SwapChain();
  renderable Texture2DView<float>* GetCurrentTextureView();
  void Present();
}

native class Math {
 ~Math();
  static float sqrt(float v);
  static float rand();
  static float sin(float v);
  static float cos(float v);
  static float abs(float v);
  static float<3> normalize(float<3> v);
  static float<3> reflect(float<3> incident, float<3> normal);
  static float<3> refract(float<3> incident, float<3> normal, float eta);
  static float<4,4> inverse(float<4,4> m);
  static float<4,4> transpose(float<4,4> m);
}

native class ImageDecoder<PF> {
  ImageDecoder(ubyte[]^ encodedImage);
 ~ImageDecoder();
  uint Width();
  uint Height();
  void Decode(writeonly PF::MemoryType[]^ buffer, uint bufferWidth);
}

enum EventType { MouseMove, MouseDown, MouseUp, Unknown }

enum EventModifiers { Shift = 0x01, Control = 0x02, Alt = 0x04 }

native class Event {
 ~Event();
  EventType   type;
  int<2>      position;
  uint        button;
  uint        modifiers;
}

native class System {
 ~System();
  static bool   IsRunning();
  static bool   HasPendingEvents();
  static Event* GetNextEvent();
  static int    StorageBarrier();   // FIXME should be void return
  static double GetCurrentTime();
}

class VertexBuiltins {
  readonly int       vertexIndex;
  readonly int       instanceIndex;
  writeonly float<4> position;
  writeonly float    pointSize;
}

class FragmentBuiltins {
  readonly float<4>  fragCoord;
  readonly bool      frontFacing;
//  readonly uint      sampleIndex;
//  readonly uint      sampleMaskIn;
//  writeonly uint     sampleMaskOut;
//  writeonly float    fragDepth;
}

class ComputeBuiltins {
  readonly uint<3>   localInvocationId;
  readonly uint      localInvocationIndex;
  readonly uint<3>   globalInvocationId;
  readonly uint<3>   workgroupId;
//  readonly uint<3>   numWorkgroups;
}

class PixelFormat<SampledType, MemoryType> {}

class R8unorm : PixelFormat<float, ubyte> {}
class R8snorm : PixelFormat<float, byte> {}
class R8uint : PixelFormat<uint, ubyte> {}
class R8sint : PixelFormat<int, byte> {}

class RG8unorm : PixelFormat<float, ubyte<2>> {}
class RG8snorm : PixelFormat<float, byte<2>> {}
class RG8uint : PixelFormat<uint, ubyte<2>> {}
class RG8sint : PixelFormat<int, byte<2>> {}

class RGBA8unorm : PixelFormat<float, ubyte<4>> {}
class RGBA8unormSRGB : PixelFormat<float, ubyte<4>> {}
class RGBA8snorm : PixelFormat<float, byte<4>> {}
class RGBA8uint : PixelFormat<uint, ubyte<4>> {}
class RGBA8sint : PixelFormat<int, byte<4>> {}

class BGRA8unorm : PixelFormat<float, ubyte<4>> {}
class BGRA8unormSRGB : PixelFormat<float, ubyte<4>> {}

class R16uint : PixelFormat<uint, ushort> {}
class R16sint : PixelFormat<int, short> {}
// class R16float : PixelFormat<float, half> {}

class RG16uint : PixelFormat<uint, ushort<2>> {}
class RG16sint : PixelFormat<int, short<2>> {}
// class RG16float : PixelFormat<float, half<2>> {}

class RGBA16uint : PixelFormat<uint, ushort<4>> {}
class RGBA16sint : PixelFormat<int, short<4>> {}
// class RGBA16float : PixelFormat<float, half<4>> {}

class R32uint : PixelFormat<uint, uint> {}
class R32sint : PixelFormat<int, int> {}
class R32float : PixelFormat<float, float> {}

class RG32uint : PixelFormat<uint, uint<2>> {}
class RG32sint : PixelFormat<int, int<2>> {}
class RG32float : PixelFormat<float, float<2>> {}

class RGBA32uint : PixelFormat<uint, uint<4>> {}
class RGBA32sint : PixelFormat<int, int<4>> {}
class RGBA32float : PixelFormat<float, float<4>> {}

class RGB10A2unorm : PixelFormat<float, uint> {}
class RG11B10ufloat : PixelFormat<float, uint> {}

class Depth24Plus : PixelFormat<float, uint> {}
