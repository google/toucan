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

enum FrontFace { CCW, CW }

enum CullMode { None, Front, Back }

class CommandBuffer {
 ~CommandBuffer();
}

class Queue {
 ~Queue();
  Submit(commandBuffer : &CommandBuffer);
}

class Device {
  Device();
 ~Device();
  GetQueue() : *Queue;
}

class CommandEncoder;

class Buffer<T> {
  Buffer(device : &Device, size : uint = 1u);
  Buffer(device : &Device, t : &T);
 ~Buffer();
  SetData(data : &T);
  CopyFromBuffer(encoder : &CommandEncoder, source : &Buffer<T>);
  deviceonly MapRead() uniform : *readonly uniform T;
  deviceonly MapWrite() writeonly storage : *writeonly storage T;
  deviceonly Map() storage : *storage T;
  MapRead() hostreadable : *readonly T;
  MapWrite() hostwriteable : *writeonly T;
}

class DepthStencilState {
  var depthWriteEnabled = false;
  var stencilReadMask = 0xFFFFFFFF;
  var stencilWriteMask = 0xFFFFFFFF;
  var depthBias = 0;
  var depthBiasSlopeScale = 0.0;
  var depthBiasClamp = 0.0;
}

enum BlendOp {
  Add,
  Subtract,
  ReverseSubtract,
  Min,
  Max
}

enum BlendFactor {
  Zero,
  One,
  Src,
  OneMinusSrc,
  SrcAlpha,
  OneMinusSrcAlpha,
  Dst,
  OneMinusDst,
  DstAlpha,
  OneMinusDstAlpha,
  SrcAlphaSaturated,
  Constant,
  OneMinusConstant
}

class BlendComponent {
  var operation = BlendOp.Add;
  var srcFactor = BlendFactor.One;
  var dstFactor = BlendFactor.Zero;
}

class BlendState {
  var color : BlendComponent;
  var alpha : BlendComponent;
}

class RenderPipeline<T> {
  RenderPipeline(device : &Device, primitiveTopology = PrimitiveTopology.TriangleList, frontFace = FrontFace.CCW, cullMode = CullMode.None, depthStencilState : &DepthStencilState = {}, blendState : &BlendState = {});
 ~RenderPipeline();
}

class ComputePipeline<T> {
  ComputePipeline(device : &Device);
 ~ComputePipeline();
}

class BindGroup<T> {
  BindGroup(device : &Device, data : &T);
 ~BindGroup();
  deviceonly Get() : T;
}

enum LoadOp {
  Undefined,
  Clear,
  Load
}

enum StoreOp {
  Undefined,
  Store,
  Discard
}

class VertexInput<T> {
  VertexInput(buffer : &vertex Buffer<[]T>);
  deviceonly Get() : T;
 ~VertexInput();
}

class ColorAttachment<PF> {
  deviceonly Set(value : PF:DeviceType<4>);
 ~ColorAttachment();
}

class DepthStencilAttachment<PF> {
 ~DepthStencilAttachment();
}

enum AddressMode { Repeat, MirrorRepeat, ClampToEdge };

enum FilterMode { Nearest, Linear };

class Sampler {
  Sampler(device : &Device, addressModeU = AddressMode.ClampToEdge, addressModeV = AddressMode.ClampToEdge, addressModeW = AddressMode.ClampToEdge, magFilter = FilterMode.Linear, minFilter = FilterMode.Linear, mipmapFilter = FilterMode.Linear);
 ~Sampler();
}

class SampleableTexture1D<ST> {
 ~SampleableTexture1D();
  deviceonly Sample(sampler : &Sampler, coord : float) : ST<4>;
  deviceonly Load(coord : uint, level : uint) : ST<4>;
  deviceonly GetSize() : uint;
}

class SampleableTexture2D<ST> {
 ~SampleableTexture2D();
  deviceonly Sample(sampler : &Sampler, coords : float<2>) : ST<4>;
  deviceonly Load(coord : uint<2>, level : uint) : ST<4>;
  deviceonly GetSize() : uint<2>;
}

class SampleableTexture2DArray<ST> {
 ~SampleableTexture2DArray();
  deviceonly Sample(sampler : &Sampler, coords : float<2>, layer : uint) : ST<4>;
  deviceonly Load(coord : uint<2>, layer : uint, level : uint) : ST<4>;
  deviceonly GetSize() : uint<2>;
}

class SampleableTexture3D<ST> {
 ~SampleableTexture3D();
  deviceonly Sample(sampler : &Sampler, coords : float<3>) : ST<4>;
  deviceonly Load(coord : uint<3>, level : uint) : ST<4>;
  deviceonly GetSize() : uint<3>;
}

class SampleableTextureCube<ST> {
 ~SampleableTextureCube();
  deviceonly Sample(sampler : &Sampler, coords : float<3>) : ST<4>;
  deviceonly GetSize() : uint<2>;
}

class Texture1D<PF> {
  Texture1D(device : &Device, width : uint);
 ~Texture1D();
  CreateSampleableView() sampleable : *SampleableTexture1D<PF:DeviceType>;
  CreateStorageView(mipLevel = 0u) : *storage Texture1D<PF>;
  CopyFromBuffer(encoder : &CommandEncoder, source : &Buffer<[]PF:HostType>, width : uint, origin : uint = 0);
}

class Texture2D<PF> {
  Texture2D(device : &Device, size : uint<2>);
 ~Texture2D();
  CreateSampleableView() sampleable : *SampleableTexture2D<PF:DeviceType>;
  CreateRenderableView(mipLevel = 0u) : *renderable Texture2D<PF>;
  CreateStorageView(mipLevel = 0u) : *storage Texture2D<PF>;
  CreateColorAttachment(loadOp = LoadOp.Load, storeOp = StoreOp.Store, clearValue = float<4>(0.0, 0.0, 0.0, 0.0)) renderable : *ColorAttachment<PF>;
  CreateDepthStencilAttachment(depthLoadOp = LoadOp.Load, depthStoreOp = StoreOp.Store, depthClearValue = 1.0, stencilLoadOp = LoadOp.Undefined, stencilStoreOp = StoreOp.Undefined, stencilClearValue = 0) renderable : *DepthStencilAttachment<PF>;
  MinBufferWidth() : uint;
  CopyFromBuffer(encoder : &CommandEncoder, source : &Buffer<[]PF:HostType>, size : uint<2>, origin : uint<2> = uint<2>(0, 0));
}

class Texture2DArray<PF> {
  Texture2DArray(device : &Device, size : uint<3>);
 ~Texture2D();
  CreateSampleableView() sampleable : *SampleableTexture2DArray<PF:DeviceType>;
  CreateRenderableView(layee : uint, mipLevel = 0u) : *renderable Texture2D<PF>;
  CreateStorageView(layer : uint, mipLevel = 0u) : *storage Texture2DArray<PF>;
  MinBufferWidth() : uint;
  CopyFromBuffer(encoder : &CommandEncoder, source : &Buffer<[]PF:HostType>, size : uint<3>, origin = uint<3>(0, 0, 0));
}

class Texture3D<PF> {
  Texture3D(device : &Device, size : uint<3>);
 ~Texture3D();
  CreateSampleableView() sampleable : *SampleableTexture3D<PF:DeviceType>;
  CreateRenderableView(depth : uint, mipLevel = 0u) : *renderable Texture2D<PF>;
  CreateStorageView(depth : uint, mipLevel = 0u) : *storage Texture3D<PF>;
  MinBufferWidth() : uint;
  CopyFromBuffer(encoder : &CommandEncoder, source : &Buffer<[]PF:HostType>, size : uint<3>, origin = uint<3>(0, 0, 0));
}

class TextureCube<PF> {
  TextureCube(device : &Device, size : uint<2>);
 ~TextureCube();
  CreateSampleableView() sampleable : *SampleableTextureCube<PF:DeviceType>;
  CreateRenderableView(face : uint, mipLevel = 0u) : *renderable Texture2D<PF>;
  CreateStorageView(face : uint, mipLevel = 0u) : *storage Texture2D<PF>;
  MinBufferWidth() : uint;
  CopyFromBuffer(encoder : &CommandEncoder, source : &Buffer<[]PF:HostType>, size : uint<3>, origin = uint<3>(0, 0, 0));
}

class CommandEncoder {
  CommandEncoder(device : &Device);
 ~CommandEncoder();
  Finish() : *CommandBuffer;
}

class RenderPass<T> {
  RenderPass(encoder : &CommandEncoder, data : &T);
  RenderPass(base : &RenderPass<T:BaseClass>);
 ~RenderPass();
  Draw(vertexCount : uint, instanceCount : uint, firstVertex : uint, firstInstance : uint);
  DrawIndexed(indexCount : uint, instanceCount : uint, firstIndex : uint, baseVertex : uint, firstIntance : uint);
  SetPipeline(pipeline : &RenderPipeline<T>);
  Set(data : &T);
  End();
}

class ComputePass<T> {
  ComputePass(encoder : &CommandEncoder, data : &T);
  ComputePass(base : &ComputePass<T:BaseClass>);
 ~ComputePass();
  Dispatch(workgroupCountX : uint, workgroupCountY : uint, workgroupCountZ : uint);
  SetPipeline(pipeline : &ComputePipeline<T>);
  Set(data : &T);
  End();
}

class Window {
  Window(size : uint<2>, position = int<2>(0, 0));
  GetSize() : uint<2>;
 ~Window();
}

class SwapChain<T> {
  SwapChain(device : &Device, window : &Window);
 ~SwapChain();
  Resize(size : uint<2>);
  GetCurrentTexture() : *renderable Texture2D<T>;
  Present();
}

class Math {
 ~Math();
  static all(v : bool<2>)   : bool;
  static all(v : bool<3>)   : bool;
  static all(v : bool<4>)   : bool;
  static any(v : bool<2>)   : bool;
  static any(v : bool<3>)   : bool;
  static any(v : bool<4>)   : bool;
  static sqrt(v : float)    : float;
  static sqrt(v : float<2>) : float<2>;
  static sqrt(v : float<3>) : float<3>;
  static sqrt(v : float<4>) : float<4>;
  static sin(v : float)     : float;
  static sin(v : float<2>)  : float<2>;
  static sin(v : float<3>)  : float<3>;
  static sin(v : float<4>)  : float<4>;
  static cos(v : float)     : float;
  static cos(v : float<2>)  : float<2>;
  static cos(v : float<3>)  : float<3>;
  static cos(v : float<4>)  : float<4>;
  static tan(v : float)     : float;
  static tan(v : float<2>)  : float<2>;
  static tan(v : float<3>)  : float<3>;
  static tan(v : float<4>)  : float<4>;
  static dot(v1 : float<2>, v2 : float<2>) : float;
  static dot(v1 : float<3>, v2 : float<3>) : float;
  static dot(v1 : float<4>, v2 : float<4>) : float;
  static cross(v1 : float<3>, v2 : float<3>) : float<3>;
  static fabs(v : float)    : float;
  static fabs(v : float<2>) : float<2>;
  static fabs(v : float<3>) : float<3>;
  static fabs(v : float<4>) : float<4>;
  static floor(v : float)   : float;
  static floor(v : float<2>) : float<2>;
  static floor(v : float<3>) : float<3>;
  static floor(v : float<4>) : float<4>;
  static ceil(v : float)   : float;
  static ceil(v : float<2>) : float<2>;
  static ceil(v : float<3>) : float<3>;
  static ceil(v : float<4>) : float<4>;
  static min(v1 : float,    v2 : float) : float;
  static min(v1 : float<2>, v2 : float<2>) : float<2>;
  static min(v1 : float<3>, v2 : float<3>) : float<3>;
  static min(v1 : float<4>, v2 : float<4>) : float<4>;
  static max(v1 : float,    v2 : float) : float;
  static max(v1 : float<2>, v2 : float<2>) : float<2>;
  static max(v1 : float<3>, v2 : float<3>) : float<3>;
  static max(v1 : float<4>, v2 : float<4>) : float<4>;
  static length(v : float) : float;
  static length(v : float<2>) : float;
  static length(v : float<3>) : float;
  static length(v : float<4>) : float;
  static pow(v1 : float,    v2 : float) : float;
  static pow(v1 : float<2>, v2 : float<2>) : float<2>;
  static pow(v1 : float<3>, v2 : float<3>) : float<3>;
  static pow(v1 : float<4>, v2 : float<4>) : float<4>;
  static clz(value : int)   : int;
  static rand()             : float;
  static normalize(v : float<3>) : float<3>;
  static reflect(incident : float<3>, normal : float<3>) : float<3>;
  static refract(incident : float<3>, normal : float<3>, eta : float) : float<3>;
  static inverse(m : float<4,4>) : float<4,4>;
  static transpose(m : float<4,4>) : float<4,4>;
}

class Image<PF> {
  Image(encodedImage : *[]ubyte);
 ~Image();
  GetSize() : uint<2>;
  Decode(buffer : &writeonly []PF:HostType, bufferWidth : uint);
}

enum EventType { MouseMove, MouseDown, MouseUp, TouchStart, TouchMove, TouchEnd, Unknown }

enum EventModifiers { Shift = 0x01, Control = 0x02, Alt = 0x04 }

class Event {
 ~Event();
  var type : EventType;
  var position : int<2>;
  var button : uint;
  var modifiers : uint;
  var touches : [10]int<2>;
  var numTouches : int;
}

class System {
 ~System();
  static IsRunning() : bool;
  static HasPendingEvents() : bool;
  static GetNextEvent() : *Event;
  static GetScreenSize() : uint<2>;
  static StorageBarrier() : int;
  static GetCurrentTime() : double;
  static Print(str : &[]ubyte);
  static PrintLine(str : &[]ubyte);
  static GetSourceFile() : *[]ubyte;
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

class PixelFormat<DeviceType, HostType> {}

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
class R16float : PixelFormat<float, ushort> {}

class RG16uint : PixelFormat<uint, ushort<2>> {}
class RG16sint : PixelFormat<int, short<2>> {}
class RG16float : PixelFormat<float, ushort<2>> {}

class RGBA16uint : PixelFormat<uint, ushort<4>> {}
class RGBA16sint : PixelFormat<int, short<4>> {}
class RGBA16float : PixelFormat<float, ushort<4>> {}

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
