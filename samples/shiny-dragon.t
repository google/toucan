include "cube.t"
include "cubic.t"
include "dragon.t"
include "event-handler.t"
include "mesh.t"
include "quaternion.t"
include "transform.t"

class Vertex {
  var position : float<3>;
  var normal   : float<3>;
}

using Format = RGBA8unorm;

class CubeLoader {
  static Load(device : *Device, data : ^[]ubyte, texture : ^TextureCube<Format>, face : uint) {
    var image = new ImageDecoder<Format>(data);
    var size = image.GetSize();
    var buffer = new hostwriteable Buffer<[]Format:HostType>(device, texture.MinBufferWidth() * size.y);
    var b = buffer.MapWrite();
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

var dragon = new Mesh<Vertex, uint>(&dragonVertices, &dragonTriangles, 0.5 * 3.1415926);

class Uniforms {
  var model       : float<4,4>;
  var view        : float<4,4>;
  var projection  : float<4,4>;
  var viewInverse : float<4,4>;
}

class Bindings {
  var sampler : *Sampler;
  var textureView : *SampleableTextureCube<float>;
  var uniforms : *uniform Buffer<Uniforms>;
}

class DrawPipeline {
  var indexBuffer : *index Buffer<[]uint>;
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
  var depth : *DepthStencilAttachment<Depth24Plus>;
  var bindings : *BindGroup<Bindings>;
}

class SkyboxPipeline : DrawPipeline {
    vertex main(vb : &VertexBuiltins) : float<3> {
        var v = position.Get();
        var uniforms = bindings.Get().uniforms.MapRead();
        var pos = float<4>(v.x, v.y, v.z, 1.0);
        vb.position = uniforms.projection * uniforms.view * uniforms.model * pos;
        return v;
    }
    fragment main(fb : &FragmentBuiltins, position : float<3>) {
      var p = Math.normalize(position);
      var b = bindings.Get();
      // TODO: figure out why the skybox is X-flipped
      fragColor.Set(b.textureView.Sample(b.sampler, float<3>(-p.x, p.y, p.z)));
    }
    var position : *VertexInput<float<3>>;
};

class ReflectionPipeline : DrawPipeline {
    vertex main(vb : &VertexBuiltins) : Vertex {
        var v = vert.Get();
        var n = Math.normalize(v.normal);
        var uniforms = bindings.Get().uniforms.MapRead();
        var viewModel = uniforms.view * uniforms.model;
        var pos = viewModel * float<4>(v.position.x, v.position.y, v.position.z, 1.0);
        var normal = viewModel * float<4>(n.x, n.y, n.z, 0.0);
        vb.position = uniforms.projection * pos;
        var varyings : Vertex;
        varyings.position = float<3>(pos.x, pos.y, pos.z);
        varyings.normal = float<3>(normal.x, normal.y, normal.z);
        return varyings;
    }
    fragment main(fb : &FragmentBuiltins, varyings : Vertex) {
      var b = bindings.Get();
      var uniforms = b.uniforms.MapRead();
      var p = Math.normalize(varyings.position);
      var n = Math.normalize(varyings.normal);
      var r = Math.reflect(p, n);
      var r4 = uniforms.viewInverse * float<4>(r.x, r.y, r.z, 0.0);
      fragColor.Set(b.textureView.Sample(b.sampler, float<3>(-r4.x, r4.y, r4.z)));
    }
    var vert : *VertexInput<Vertex>;
};

var cubePipeline = new RenderPipeline<SkyboxPipeline>(device);
var cubeBindings : Bindings;
cubeBindings.uniforms = new uniform Buffer<Uniforms>(device);
cubeBindings.sampler = new Sampler(device);
cubeBindings.textureView = texture.CreateSampleableView();

var cubeData : SkyboxPipeline;
var cubeVB = new vertex Buffer<[]float<3>>(device, &cubeVerts);
cubeData.position = new VertexInput<float<3>>(cubeVB);
cubeData.indexBuffer = new index Buffer<[]uint>(device, &cubeIndices);
cubeData.bindings = new BindGroup<Bindings>(device, &cubeBindings);

var reflectionPipeline = new RenderPipeline<ReflectionPipeline>(device);
var dragonBindings : Bindings;
dragonBindings.sampler = cubeBindings.sampler;
dragonBindings.textureView = cubeBindings.textureView;
dragonBindings.uniforms = new uniform Buffer<Uniforms>(device);

var dragonVB = new vertex Buffer<[]Vertex>(device, dragon.vertices);
var reflectionData : ReflectionPipeline;
reflectionData.vert = new VertexInput<Vertex>(dragonVB);
reflectionData.indexBuffer = new index Buffer<[]uint>(device, dragon.indices);
reflectionData.bindings = new BindGroup<Bindings>(device, &dragonBindings);

var handler : EventHandler;
handler.rotation = float<2>(0.0, 0.0);
handler.distance = 10.0;
var dragonQuat = Quaternion(float<3>(1.0, 0.0, 0.0), -3.1415926 / 2.0);
dragonQuat.normalize();
var depthBuffer = new renderable Texture2D<Depth24Plus>(device, window.GetSize());
var uniforms : Uniforms;
var prevWindowSize = uint<2>{0, 0};
while (System.IsRunning()) {
  var orientation = Quaternion(float<3>(0.0, 1.0, 0.0), handler.rotation.x);
  orientation = orientation.mul(Quaternion(float<3>(1.0, 0.0, 0.0), handler.rotation.y));
  orientation.normalize();
  var newSize = window.GetSize();
  // FIXME: relationals should work on vectors
  if (newSize.x != prevWindowSize.x || newSize.y != prevWindowSize.y) {
    swapChain.Resize(newSize);
    depthBuffer = new renderable Texture2D<Depth24Plus>(device, newSize);
    var aspectRatio = (float) newSize.x / (float) newSize.y;
    uniforms.projection = Transform.projection(0.5, 200.0, -aspectRatio, aspectRatio, -1.0, 1.0);
    prevWindowSize = newSize;
  }
  uniforms.view = Transform.translate(0.0, 0.0, -handler.distance);
  uniforms.view *= orientation.toMatrix();
  uniforms.model = Transform.scale(100.0, 100.0, 100.0);
  uniforms.viewInverse = Transform.invert(uniforms.view);
  cubeBindings.uniforms.SetData(&uniforms);
  uniforms.model = Transform.scale(0.15, 0.15, 0.15) * Transform.translate(0.0, -50.0, 0.0);
  dragonBindings.uniforms.SetData(&uniforms);
  var encoder = new CommandEncoder(device);
  var fb = swapChain.GetCurrentTexture().CreateColorAttachment(LoadOp.Clear);
  var db = depthBuffer.CreateDepthStencilAttachment(LoadOp.Clear);
  var renderPass = new RenderPass<DrawPipeline>(encoder, { fragColor = fb, depth = db });

  var cubePass = new RenderPass<SkyboxPipeline>(renderPass);
  cubePass.SetPipeline(cubePipeline);
  cubePass.Set(&cubeData);
  cubePass.DrawIndexed(cubeIndices.length, 1, 0, 0, 0);

  var dragonPass = new RenderPass<ReflectionPipeline>(renderPass);
  dragonPass.SetPipeline(reflectionPipeline);
  dragonPass.Set(&reflectionData);
  dragonPass.DrawIndexed(dragon.indices.length, 1, 0, 0, 0);

  renderPass.End();
  var cb = encoder.Finish();
  device.GetQueue().Submit(cb);
  swapChain.Present();

  do {
    handler.Handle(System.GetNextEvent());
  } while (System.HasPendingEvents());
}
