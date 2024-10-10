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
  Submit(CommandBuffer* commandBuffer);
}

native class Device {
  Device();
 ~Device();
  GetQueue() : Queue*;
}

native class Buffer<T> {
  Buffer(Device* device, uint size = 1);
  Buffer(Device* device, T^ t);
 ~Buffer();
  SetData(T^ data);
  deviceonly Get() : T::ElementType;
  MapRead() : readonly T^;
  MapWrite() : writeonly T^;
  deviceonly MapReadUniform() : readonly uniform T^;
  deviceonly MapWriteStorage() : writeonly storage T^;
  deviceonly MapReadWriteStorage() : storage T^;
  Unmap();
}

class DepthStencilState<T> {
  var depthWriteEnabled : bool = false;
  var stencilReadMask : int = 0xFFFFFFFF;
  var stencilWriteMask : int = 0xFFFFFFFF;
  var depthBias : int = 0;
  var depthBiasSlopeScale : float = 0.0;
  var depthBiasClamp : float = 0.0;
}

native class RenderPipeline<T> {
  RenderPipeline(Device* device, DepthStencilState^ depthStencilState, PrimitiveTopology primitiveTopology);
 ~RenderPipeline();
}

native class ComputePipeline<T> {
  ComputePipeline(Device* device);
 ~ComputePipeline();
}

native class BindGroup<T> {
  BindGroup(Device* device, T^ data);
 ~BindGroup();
  deviceonly Get() : T;
}

enum AddressMode { Repeat, MirrorRepeat, ClampToEdge };

enum FilterMode { Nearest, Linear };

native class Sampler {
  Sampler(Device* device, AddressMode addressModeU, AddressMode addressModeV, AddressMode addressModeW, FilterMode magFilter, FilterMode minFilter, FilterMode mipmapFilter);
 ~Sampler();
}

native class SampleableTexture1D<ST> {
 ~SampleableTexture1D();
  deviceonly Sample(Sampler* sampler, float coord) : ST<4>;
}

native class SampleableTexture2D<ST> {
 ~SampleableTexture2D();
  deviceonly Sample(Sampler* sampler, float<2> coords) : ST<4>;
}

native class SampleableTexture2DArray<ST> {
 ~SampleableTexture2DArray();
  deviceonly Sample(Sampler* sampler, float<2> coords, uint layer) : ST<4>;
}

native class SampleableTexture3D<ST> {
 ~SampleableTexture3D();
  deviceonly Sample(Sampler* sampler, float<3> coords) : ST<4>;
}

native class SampleableTextureCube<ST> {
 ~SampleableTextureCube();
  deviceonly Sample(Sampler* sampler, float<3> coords) : ST<4>;
}

native class CommandEncoder;

native class Texture1D<PF> {
  Texture1D(Device* device, uint width);
 ~Texture1D();
  CreateSampleableView() : SampleableTexture1D<PF::SampledType>*;
  CreateStorageView(uint mipLevel = 0) : storage Texture1D<PF>*;
  CopyFromBuffer(CommandEncoder^ encoder, Buffer<PF::MemoryType[]>^ source, uint width, uint origin = 0);
}

native class Texture2D<PF> {
  Texture2D(Device* device, uint<2> size);
 ~Texture2D();
  CreateSampleableView() : SampleableTexture2D<PF::SampledType>*;
  CreateRenderableView(uint mipLevel = 0) : renderable Texture2D<PF>*;
  CreateStorageView(uint mipLevel = 0) : storage Texture2D<PF>*;
  MinBufferWidth() : uint;
  CopyFromBuffer(CommandEncoder^ encoder, Buffer<PF::MemoryType[]>^ source, uint<2> size, uint<2> origin = uint<2>(0, 0));
}

native class Texture2DArray<PF> {
  Texture2DArray(Device* device, uint<3> size);
 ~Texture2D();
  CreateSampleableView() : SampleableTexture2DArray<PF::SampledType>*;
  CreateRenderableView(uint layer, uint mipLevel = 0) : renderable Texture2D<PF>*;
  CreateStorageView(uint layer, uint mipLevel = 0) : storage Texture2DArray<PF>*;
  MinBufferWidth() : uint;
  CopyFromBuffer(CommandEncoder^ encoder, Buffer<PF::MemoryType[]>^ source, uint<3> size, uint<3> origin = uint<3>(0, 0, 0));
}

native class Texture3D<PF> {
  Texture3D(Device* device, uint<3> size);
 ~Texture3D();
  CreateSampleableView() : SampleableTexture3D<PF::SampledType>*;
  CreateRenderableView(uint depth, uint mipLevel = 0) : renderable Texture2D<PF>*;
  CreateStorageView(uint depth, uint mipLevel = 0) : storage Texture3D<PF>*;
  MinBufferWidth() : uint;
  CopyFromBuffer(CommandEncoder^ encoder, Buffer<PF::MemoryType[]>^ source, uint<3> size, uint<3> origin = uint<3>(0, 0, 0));
}

native class TextureCube<PF> {
  TextureCube(Device* device, uint<2> size);
 ~TextureCube();
  CreateSampleableView() : SampleableTextureCube<PF::SampledType>*;
  CreateRenderableView(uint face, uint mipLevel = 0) : renderable Texture2D<PF>*;
  CreateStorageView(uint face, uint mipLevel = 0) : storage Texture2D<PF>*;
  MinBufferWidth() : uint;
  CopyFromBuffer(CommandEncoder^ encoder, Buffer<PF::MemoryType[]>^ source, uint<3> size, uint<3> origin = uint<3>(0, 0, 0));
}

native class CommandEncoder {
  CommandEncoder(Device* device);
 ~CommandEncoder();
  Finish() : CommandBuffer*;
  CopyBufferToBuffer(Buffer^ source, Buffer^ dest);
}

enum LoadOp {
  LoadUndefined,
  Clear,
  Load
}

enum StoreOp {
  StoreUndefined,
  Store,
  Discard
}

native class ColorAttachment<PF> {
  ColorAttachment(renderable Texture2D<PF>* texture, LoadOp loadOp, StoreOp storeOp, float<4> clearValue = float<4>(0.0, 0.0, 0.0, 0.0));
  deviceonly Set(PF::SampledType<4> value);
 ~ColorAttachment();
}

native class DepthStencilAttachment<PF> {
  DepthStencilAttachment(renderable Texture2D<PF>* texture, LoadOp depthLoadOp, StoreOp depthStoreOp, float depthClearValue = 0.0, LoadOp stencilLoadOp, StoreOp stencilStoreOp, int stencilClearValue);
 ~DepthStencilAttachment();
}

native class RenderPass<T> {
  RenderPass(CommandEncoder* encoder, T^ data);
  RenderPass(RenderPass<T::BaseClass>^ base);
 ~RenderPass();
  Draw(uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance);
  DrawIndexed(uint indexCount, uint instanceCount, uint firstIndex, uint baseVertex, uint firstIntance);
  SetPipeline(RenderPipeline<T>* pipeline);
  Set(T^ data);
  End();
}

native class ComputePass<T> {
  ComputePass(CommandEncoder* encoder, T^ data);
 ~ComputePass();
  Dispatch(uint workgroupCountX, uint workgroupCountY, uint workgroupCountZ);
  SetPipeline(ComputePipeline* pipeline);
  Set(T^ data);
  End();
}

native class Window {
  Window(int<2> position, uint<2> size);
  GetSize() : uint<2>;
 ~Window();
}

native class SwapChain<T> {
  SwapChain(Device^ device, Window^ window);
 ~SwapChain();
  Resize(uint<2> size);
  GetCurrentTexture() : renderable Texture2D<T>*;
  Present();
}

native class Math {
 ~Math();
  static sqrt(float v)    : float;
  static sqrt(float<2> v) : float<2>;
  static sqrt(float<3> v) : float<3>;
  static sqrt(float<4> v) : float<4>;
  static sin(float v)     : float;
  static sin(float<2> v)  : float<2>;
  static sin(float<3> v)  : float<3>;
  static sin(float<4> v)  : float<4>;
  static cos(float v)     : float;
  static cos(float<2> v)  : float<2>;
  static cos(float<3> v)  : float<3>;
  static cos(float<4> v)  : float<4>;
  static fabs(float v)    : float;
  static fabs(float<2> v) : float<2>;
  static fabs(float<3> v) : float<3>;
  static fabs(float<4> v) : float<4>;
  static clz(int value)   : int;
  static rand()           : float;
  static normalize(float<3> v) : float<3>;
  static reflect(float<3> incident, float<3> normal) : float<3>;
  static refract(float<3> incident, float<3> normal, float eta) : float<3>;
  static inverse(float<4,4> m) : float<4,4>;
  static transpose(float<4,4> m) : float<4,4>;
}

native class ImageDecoder<PF> {
  ImageDecoder(ubyte[]^ encodedImage);
 ~ImageDecoder();
  GetSize() : uint<2>;
  Decode(writeonly PF::MemoryType[]^ buffer, uint bufferWidth);
}

enum EventType { MouseMove, MouseDown, MouseUp, TouchStart, TouchMove, TouchEnd, Unknown }

enum EventModifiers { Shift = 0x01, Control = 0x02, Alt = 0x04 }

native class Event {
 ~Event();
  var type : EventType;
  var position : int<2>;
  var button : uint;
  var modifiers : uint;
  var touches : int<2>[10];
  var numTouches : int;
}

native class System {
 ~System();
  static IsRunning() : bool;
  static HasPendingEvents() : bool;
  static GetNextEvent() : Event*;
  static GetScreenSize() : uint<2>;
  static StorageBarrier() : int;
  static GetCurrentTime() : double;
  static Print(ubyte[]^ str);
  static PrintLine(ubyte[]^ str);
  static GetSourceFile() : ubyte[]*;
  static GetSourceLine() : int;
  static Abort();
}

class VertexBuiltins {
  var vertexIndex : readonly int;
  var instanceIndex : readonly int;
  var position : writeonly float<4>;
}

class FragmentBuiltins {
  var fragCoord : readonly float<4>;
  var frontFacing : readonly bool;
}

class ComputeBuiltins {
  var localInvocationId : readonly uint<3>;
  var localInvocationIndex : readonly uint;
  var globalInvocationId : readonly uint<3>;
  var workgroupId : readonly uint<3>;
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

class PreferredSwapChainFormat : PixelFormat<float, ubyte<4>> {}
