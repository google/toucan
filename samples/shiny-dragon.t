include "cube.t"
include "cube-loader.t"
include "cubic.t"
include "dragon.t"
include "event-handler.t"
include "mesh.t"
include "mipmap-generator.t"
include "quaternion.t"
include "transform.t"

class Vertex {
  var position : float<3>;
  var normal   : float<3>;
}

var device = new Device();

var texture = new sampleable renderable TextureCube<RGBA8unorm>(device, {2176, 2176}, 12);
var loader = CubeLoader<RGBA8unorm>{device, texture};
loader.Load(inline("third_party/home-cube/right.jpg"), 0);
loader.Load(inline("third_party/home-cube/left.jpg"), 1);
loader.Load(inline("third_party/home-cube/top.jpg"), 2);
loader.Load(inline("third_party/home-cube/bottom.jpg"), 3);
loader.Load(inline("third_party/home-cube/front.jpg"), 4);
loader.Load(inline("third_party/home-cube/back.jpg"), 5);

MipmapGenerator<RGBA8unorm>.Generate(device, texture);

var window = new Window(System.GetScreenSize());
var swapChain = new SwapChain<PreferredPixelFormat>(device, window);

var dragon = new Mesh<Vertex, uint>(&dragonVertices, &dragonTriangles, 0.5 * Math.pi);

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
  var fragColor : *ColorOutput<PreferredPixelFormat>;
  var depth : *DepthStencilOutput<Depth24Plus>;
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
      var invR = uniforms.viewInverse * float<4>(@r, 0.0);
      fragColor.Set(b.textureView.Sample(b.sampler, float<3>(-invR.x, invR.y, invR.z)));
    }
    var vert : *VertexInput<Vertex>;
};

var cubePipeline = new RenderPipeline<SkyboxPipeline>(device);
var cubeBindings = Bindings{
  uniforms = new uniform Buffer<Uniforms>(device),
  sampler = new Sampler(device),
  textureView = texture.CreateSampleableView()
};

var cubeVB = new vertex Buffer<[]float<3>>(device, &cubeVerts);
var cubeData = SkyboxPipeline{
  position = new VertexInput<float<3>>(cubeVB),
  indexBuffer = new index Buffer<[]uint>(device, &cubeIndices),
  bindings = new BindGroup<Bindings>(device, &cubeBindings)
};

var reflectionPipeline = new RenderPipeline<ReflectionPipeline>(device);
var dragonBindings = Bindings{
  sampler = cubeBindings.sampler,
  textureView = cubeBindings.textureView,
  uniforms = new uniform Buffer<Uniforms>(device)
};

var dragonVB = new vertex Buffer<[]Vertex>(device, dragon.vertices);
var reflectionData = ReflectionPipeline{
  vert = new VertexInput<Vertex>(dragonVB),
  indexBuffer = new index Buffer<[]uint>(device, dragon.indices),
  bindings = new BindGroup<Bindings>(device, &dragonBindings)
};

var handler = EventHandler{ distance = 10.0 };
var dragonQuat = Quaternion(float<3>(1.0, 0.0, 0.0), -Math.pi / 2.0);
dragonQuat.normalize();
var depthBuffer = new renderable Texture2D<Depth24Plus>(device, window.GetSize());
var uniforms : Uniforms;
var prevWindowSize = uint<2>{0, 0};
while (System.IsRunning()) {
  var orientation = Quaternion(float<3>(0.0, 1.0, 0.0), handler.rotation.x);
  orientation = orientation.mul(Quaternion(float<3>(1.0, 0.0, 0.0), handler.rotation.y));
  orientation.normalize();
  var newSize = window.GetSize();
  if (Math.any(newSize != prevWindowSize)) {
    swapChain.Resize(newSize);
    depthBuffer = new renderable Texture2D<Depth24Plus>(device, newSize);
    var aspectRatio = newSize.x as float / newSize.y as float;
    uniforms.projection = Transform.projection(0.5, 200.0, -aspectRatio, aspectRatio, -1.0, 1.0);
    prevWindowSize = newSize;
  }
  uniforms.view = Transform.translation({0.0, 0.0, -handler.distance});
  uniforms.view *= orientation.toMatrix();
  uniforms.model = Transform.scale({100.0, 100.0, 100.0});
  uniforms.viewInverse = Transform.invert(uniforms.view);
  cubeBindings.uniforms.SetData(&uniforms);
  uniforms.model = Transform.scale({0.15, 0.15, 0.15}) * Transform.translation({0.0, -50.0, 0.0});
  dragonBindings.uniforms.SetData(&uniforms);
  var encoder = new CommandEncoder(device);
  var fb = swapChain.GetCurrentTexture().CreateColorOutput(LoadOp.Clear);
  var db = depthBuffer.CreateDepthStencilOutput(LoadOp.Clear);
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
