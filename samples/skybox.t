include "cube.t"
include "event-handler.t"
include "quaternion.t"
include "transform.t"

class Vertex {
  float<3> position;
  float<3> normal;
}

using Format = RGBA8unorm;

class CubeLoader {
  static void Load(Device* device, ubyte[]^ data, TextureCube<Format>^ texture, uint face) {
    var image = new ImageDecoder<Format>(data);
    var size = image.GetSize();
    var buffer = new Buffer<Format::MemoryType[]>(device, texture.MinBufferWidth() * size.y);
    writeonly Format::MemoryType[]^ b = buffer.MapWrite();
    image.Decode(b, texture.MinBufferWidth());
    buffer.Unmap();
    var encoder = new CommandEncoder(device);
    texture.CopyFromBuffer(encoder, buffer, {size.x, size.y, 1}, uint<3>(0, 0, face));
    device.GetQueue().Submit(encoder.Finish());
  }
}

Device* device = new Device();

var texture = new sampleable TextureCube<RGBA8unorm>(device, {2176, 2176});
CubeLoader.Load(device, inline("third_party/home-cube/right.jpg"), texture, 0);
CubeLoader.Load(device, inline("third_party/home-cube/left.jpg"), texture, 1);
CubeLoader.Load(device, inline("third_party/home-cube/top.jpg"), texture, 2);
CubeLoader.Load(device, inline("third_party/home-cube/bottom.jpg"), texture, 3);
CubeLoader.Load(device, inline("third_party/home-cube/front.jpg"), texture, 4);
CubeLoader.Load(device, inline("third_party/home-cube/back.jpg"), texture, 5);

Window* window = new Window({0, 0}, System.GetScreenSize());
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);

var cubeVB = new vertex Buffer<float<3>[]>(device, &cubeVerts);
var cubeIB = new index Buffer<uint[]>(device, &cubeIndices);

class Uniforms {
  float<4,4>  model, view, projection;
}

class Bindings {
  Sampler* sampler;
  SampleableTextureCube<float>* textureView;
  uniform Buffer<Uniforms>* uniforms;
}

class SkyboxPipeline {
    float<3> vertexShader(VertexBuiltins^ vb) vertex {
        var v = vertices.Get();
        var uniforms = bindings.Get().uniforms.MapReadUniform();
        var pos = float<4>(v.x, v.y, v.z, 1.0);
        vb.position = uniforms.projection * uniforms.view * uniforms.model * pos;
        return v;
    }
    void fragmentShader(FragmentBuiltins^ fb, float<3> position) fragment {
      float<3> p = Math.normalize(position);
      var b = bindings.Get();
      // TODO: figure out why the skybox is X-flipped
      fragColor.Set(b.textureView.Sample(b.sampler, float<3>(-p.x, p.y, p.z)));
    }
    vertex Buffer<float<3>[]>* vertices;
    index Buffer<uint[]>* indices;
    ColorAttachment<PreferredSwapChainFormat>* fragColor;
    DepthStencilAttachment<Depth24Plus>* depth;
    BindGroup<Bindings>* bindings;
};

var depthState = new DepthStencilState<Depth24Plus>();

var cubePipeline = new RenderPipeline<SkyboxPipeline>(device, depthState, TriangleList);
var cubeBindings = new Bindings();
cubeBindings.uniforms = new uniform Buffer<Uniforms>(device);
cubeBindings.sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);
cubeBindings.textureView = texture.CreateSampleableView();
var cubeBindGroup = new BindGroup<Bindings>(device, cubeBindings);

EventHandler handler;
handler.rotation = float<2>(0.0, 0.0);
handler.distance = 10.0;
var windowSize = window.GetSize();
float aspectRatio = (float) windowSize.x / (float) windowSize.y;
float<4, 4> projection = Transform.projection(0.5, 200.0, -aspectRatio, aspectRatio, -1.0, 1.0);
var depthBuffer = new renderable Texture2D<Depth24Plus>(device, window.GetSize());
while (System.IsRunning()) {
  Quaternion orientation = Quaternion(float<3>(0.0, 1.0, 0.0), handler.rotation.x);
  orientation = orientation.mul(Quaternion(float<3>(1.0, 0.0, 0.0), handler.rotation.y));
  orientation.normalize();
  Uniforms uniforms;
  uniforms.projection = projection;
  uniforms.view = Transform.translate(0.0, 0.0, -handler.distance);
  uniforms.view *= orientation.toMatrix();
  uniforms.model = Transform.scale(100.0, 100.0, 100.0);
  cubeBindings.uniforms.SetData(&uniforms);
  var encoder = new CommandEncoder(device);
  SkyboxPipeline p;
  var fb = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
  var db = new DepthStencilAttachment<Depth24Plus>(depthBuffer, Clear, Store, 1.0, LoadUndefined, StoreUndefined, 0);
  var renderPass = new RenderPass<SkyboxPipeline>(encoder,
    { fragColor = fb, depth = db, vertices = cubeVB, indices = cubeIB, bindings = cubeBindGroup }
  );

  renderPass.SetPipeline(cubePipeline);
  renderPass.DrawIndexed(cubeIndices.length, 1, 0, 0, 0);

  renderPass.End();
  CommandBuffer* cb = encoder.Finish();
  device.GetQueue().Submit(cb);
  swapChain.Present();

  do {
    handler.Handle(System.GetNextEvent());
  } while (System.HasPendingEvents());
}
