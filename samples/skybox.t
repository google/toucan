include "cube.t"
include "event-handler.t"
include "quaternion.t"
include "transform.t"

class Vertex {
  var position : float<3>;
  var normal : float<3>;
}

using Format = RGBA8unorm;

class CubeLoader {
  static Load(device : *Device, data : ^[]ubyte, texture : ^TextureCube<Format>, face : uint) {
    var image = new ImageDecoder<Format>(data);
    var size = image.GetSize();
    var buffer = new writeonly Buffer<[]Format:HostType>(device, texture.MinBufferWidth() * size.y);
    var b = buffer.Map();
    image.Decode(b, texture.MinBufferWidth());
    buffer.Unmap();
    var encoder = new CommandEncoder(device);
    texture.CopyFromBuffer(encoder, buffer, {size.x, size.y, 1}, uint<3>(0, 0, face));
    device.GetQueue().Submit(encoder.Finish());
  }
}

var device = new Device();

var texture = new sampleable TextureCube<RGBA8unorm>(device, {2176, 2176});
CubeLoader.Load(device, inline("third_party/home-cube/right.jpg"), texture, 0);
CubeLoader.Load(device, inline("third_party/home-cube/left.jpg"), texture, 1);
CubeLoader.Load(device, inline("third_party/home-cube/top.jpg"), texture, 2);
CubeLoader.Load(device, inline("third_party/home-cube/bottom.jpg"), texture, 3);
CubeLoader.Load(device, inline("third_party/home-cube/front.jpg"), texture, 4);
CubeLoader.Load(device, inline("third_party/home-cube/back.jpg"), texture, 5);

var window = new Window({0, 0}, System.GetScreenSize());
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);

var cubeVB = new vertex Buffer<[]float<3>>(device, &cubeVerts);
var cubeIB = new index Buffer<[]uint>(device, &cubeIndices);

class Uniforms {
  var model       : float<4,4>;
  var view        : float<4,4>;
  var projection  : float<4,4>;
}

class Bindings {
  var sampler : *Sampler;
  var textureView : *SampleableTextureCube<float>;
  var uniforms : *uniform Buffer<Uniforms>;
}

class SkyboxPipeline {
    vertex main(vb : ^VertexBuiltins) : float<3> {
        var v = vertices.Get();
        var uniforms = bindings.Get().uniforms.Map();
        var pos = float<4>(v.x, v.y, v.z, 1.0);
        vb.position = uniforms.projection * uniforms.view * uniforms.model * pos;
        return v;
    }
    fragment main(fb : ^FragmentBuiltins, position : float<3>) {
      var p = Math.normalize(position);
      var b = bindings.Get();
      // TODO: figure out why the skybox is X-flipped
      fragColor.Set(b.textureView.Sample(b.sampler, float<3>(-p.x, p.y, p.z)));
    }
    var vertices : *vertex Buffer<[]float<3>>;
    var indices : *index Buffer<[]uint>;
    var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
    var depth : *DepthStencilAttachment<Depth24Plus>;
    var bindings : *BindGroup<Bindings>;
};

var depthState = new DepthStencilState();

var cubePipeline = new RenderPipeline<SkyboxPipeline>(device, depthState, TriangleList);
var cubeBindings : Bindings;
cubeBindings.uniforms = new uniform Buffer<Uniforms>(device);
cubeBindings.sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);
cubeBindings.textureView = texture.CreateSampleableView();
var cubeBindGroup = new BindGroup<Bindings>(device, &cubeBindings);

var handler : EventHandler;
handler.rotation = float<2>(0.0, 0.0);
handler.distance = 10.0;
var windowSize = window.GetSize();
var aspectRatio = (float) windowSize.x / (float) windowSize.y;
var projection = Transform.projection(0.5, 200.0, -aspectRatio, aspectRatio, -1.0, 1.0);
var depthBuffer = new renderable Texture2D<Depth24Plus>(device, window.GetSize());
while (System.IsRunning()) {
  var orientation = Quaternion(float<3>(0.0, 1.0, 0.0), handler.rotation.x);
  orientation = orientation.mul(Quaternion(float<3>(1.0, 0.0, 0.0), handler.rotation.y));
  orientation.normalize();
  var uniforms : Uniforms;
  uniforms.projection = projection;
  uniforms.view = Transform.translate(0.0, 0.0, -handler.distance);
  uniforms.view *= orientation.toMatrix();
  uniforms.model = Transform.scale(100.0, 100.0, 100.0);
  cubeBindings.uniforms.SetData(&uniforms);
  var encoder = new CommandEncoder(device);
  var p : SkyboxPipeline;
  var fb = swapChain.GetCurrentTexture().CreateColorAttachment(Clear, Store);
  var db = depthBuffer.CreateDepthStencilAttachment(Clear, Store, 1.0, LoadUndefined, StoreUndefined, 0);
  var renderPass = new RenderPass<SkyboxPipeline>(encoder,
    { fragColor = fb, depth = db, vertices = cubeVB, indices = cubeIB, bindings = cubeBindGroup }
  );

  renderPass.SetPipeline(cubePipeline);
  renderPass.DrawIndexed(cubeIndices.length, 1, 0, 0, 0);

  renderPass.End();
  var cb = encoder.Finish();
  device.GetQueue().Submit(cb);
  swapChain.Present();

  do {
    handler.Handle(System.GetNextEvent());
  } while (System.HasPendingEvents());
}
