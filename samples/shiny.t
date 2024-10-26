include "cube.t"
include "cubic.t"
include "event-handler.t"
include "quaternion.t"
include "transform.t"
include "teapot.t"

class Vertex {
  var position : float<3>;
  var normal : float<3>;
}

using Format = RGBA8unorm;

class CubeLoader {
  static Load(device : *Device, data : ^[]ubyte, texture : ^TextureCube<Format>, face : uint) {
    var image = new ImageDecoder<Format>(data);
    var size = image.GetSize();
    var buffer = new writeonly Buffer<[]Format::HostType>(device, texture.MinBufferWidth() * size.y);
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

class BicubicPatch {
  Evaluate(u : float, v : float) : Vertex {
    var pu : [4]float<3>, pv : [4]float<3>;
    for (var i = 0; i < 4; ++i) {
      pu[i] = vCubics[i].Evaluate(v);
      pv[i] = uCubics[i].Evaluate(u);
    }
    var uCubic : Cubic<float<3>>;
    var vCubic : Cubic<float<3>>;
    uCubic.FromBezier(pu);
    vCubic.FromBezier(pv);
    var result : Vertex;
    result.position = uCubic.Evaluate(u);
    var uTangent = uCubic.EvaluateTangent(u);
    var vTangent = vCubic.EvaluateTangent(v);
    if (vTangent.x == 0.0 && vTangent.y == 0.0 && vTangent.z == 0.0) {
      if (result.position.z <= 0.0) {
        result.normal = { 0.0, 0.0, -1.0 };
      } else {
        result.normal = { 0.0, 0.0, 1.0 };
      }
    } else {
      result.normal = Utils.normalize(Utils.cross(vTangent, uTangent));
    }
    return result;
  }
  var uCubics : [4]Cubic<float<3>>;
  var vCubics : [4]Cubic<float<3>>;
}

class BicubicTessellator {
  BicubicTessellator(controlPoints : ^[]float<3>, controlIndices : ^[]uint, level : int) {
    var numPatches = controlIndices.length / 16;
    var patchWidth = level + 1;
    var verticesPerPatch = patchWidth * patchWidth;
    vertices = [numPatches * verticesPerPatch] new Vertex;
    indices = [numPatches * level * level * 6] new uint;
    var vi = 0, ii = 0;
    var scale = 1.0 / (float) level;
    for (var k = 0; k < controlIndices.length; k += 16) {
      var patch : BicubicPatch;
      for (var i = 0; i < 4; ++i) {
        var pu : [4]float<3>;
        var pv : [4]float<3>;
        for (var j = 0; j < 4; ++j) {
          pu[j] = controlPoints[controlIndices[k + i + j * 4]];
          pv[j] = controlPoints[controlIndices[k + i * 4 + j]];
        }
        patch.uCubics[i].FromBezier(pu);
        patch.vCubics[i].FromBezier(pv);
      }
      for (var i = 0; i <= level; ++i) {
        var v = (float) i * scale;
        for (var j = 0; j <= level; ++j) {
          var u = (float) j * scale;
          vertices[vi] = patch.Evaluate(u, v);
          if (i < level && j < level) {
            indices[ii] = vi;
            indices[ii + 1] = vi + 1;
            indices[ii + 2] = vi + patchWidth + 1;
            indices[ii + 3] = vi;
            indices[ii + 4] = vi + patchWidth + 1;
            indices[ii + 5] = vi + patchWidth;
            ii += 6;
          }
          ++vi;
        }
      }
    }
  }
  var vertices : *[]Vertex;
  var indices : *[]uint;
}

var tessTeapot = new BicubicTessellator(&teapotControlPoints, &teapotControlIndices, 8);

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
    vertex main(vb : ^VertexBuiltins) : float<3> {
        var v = position.Get();
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
    var position : *vertex Buffer<[]float<3>>;
};

class ReflectionPipeline : DrawPipeline {
    vertex main(vb : ^VertexBuiltins) : Vertex {
        var v = vert.Get();
        var n = Math.normalize(v.normal);
        var uniforms = bindings.Get().uniforms.Map();
        var viewModel = uniforms.view * uniforms.model;
        var pos = viewModel * float<4>(v.position.x, v.position.y, v.position.z, 1.0);
        var normal = viewModel * float<4>(n.x, n.y, n.z, 0.0);
        vb.position = uniforms.projection * pos;
        var varyings : Vertex;
        varyings.position = float<3>(pos.x, pos.y, pos.z);
        varyings.normal = float<3>(normal.x, normal.y, normal.z);
        return varyings;
    }
    fragment main(fb : ^FragmentBuiltins, varyings : Vertex) {
      var b = bindings.Get();
      var uniforms = b.uniforms.Map();
      var p = Math.normalize(varyings.position);
      var n = Math.normalize(varyings.normal);
      var r = Math.reflect(p, n);
      var r4 = uniforms.viewInverse * float<4>(r.x, r.y, r.z, 0.0);
      fragColor.Set(b.textureView.Sample(b.sampler, float<3>(-r4.x, r4.y, r4.z)));
    }
    var vert : *vertex Buffer<[]Vertex>;
};

var depthState = new DepthStencilState();

var cubePipeline = new RenderPipeline<SkyboxPipeline>(device, depthState, TriangleList);
var cubeBindings : Bindings;
cubeBindings.uniforms = new uniform Buffer<Uniforms>(device);
cubeBindings.sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);
cubeBindings.textureView = texture.CreateSampleableView();

var cubeData : SkyboxPipeline;
cubeData.position = new vertex Buffer<[]float<3>>(device, &cubeVerts);
cubeData.indexBuffer = new index Buffer<[]uint>(device, &cubeIndices);
cubeData.bindings = new BindGroup<Bindings>(device, &cubeBindings);

var teapotPipeline = new RenderPipeline<ReflectionPipeline>(device, depthState, TriangleList);
var teapotBindings : Bindings;
teapotBindings.sampler = cubeBindings.sampler;
teapotBindings.textureView = cubeBindings.textureView;
teapotBindings.uniforms = new uniform Buffer<Uniforms>(device);

var teapotData : ReflectionPipeline;
teapotData.vert = new vertex Buffer<[]Vertex>(device, tessTeapot.vertices);
teapotData.indexBuffer = new index Buffer<[]uint>(device, tessTeapot.indices);
teapotData.bindings = new BindGroup<Bindings>(device, &teapotBindings);

var handler : EventHandler;
handler.rotation = float<2>(0.0, 0.0);
handler.distance = 10.0;
var teapotQuat = Quaternion(float<3>(1.0, 0.0, 0.0), -3.1415926 / 2.0);
teapotQuat.normalize();
var teapotRotation = teapotQuat.toMatrix();
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
  uniforms.model = teapotRotation * Transform.scale(2.0, 2.0, 2.0);
  teapotBindings.uniforms.SetData(&uniforms);
  var encoder = new CommandEncoder(device);
  var fb = swapChain.GetCurrentTexture().CreateColorAttachment(Clear, Store);
  var db = depthBuffer.CreateDepthStencilAttachment(Clear, Store, 1.0, LoadUndefined, StoreUndefined, 0);
  var renderPass = new RenderPass<DrawPipeline>(encoder, { fragColor = fb, depth = db });

  var cubePass = new RenderPass<SkyboxPipeline>(renderPass);
  cubePass.SetPipeline(cubePipeline);
  cubePass.Set(&cubeData);
  cubePass.DrawIndexed(cubeIndices.length, 1, 0, 0, 0);

  var teapotPass = new RenderPass<ReflectionPipeline>(renderPass);
  teapotPass.SetPipeline(teapotPipeline);
  teapotPass.Set(&teapotData);
  teapotPass.DrawIndexed(tessTeapot.indices.length, 1, 0, 0, 0);

  renderPass.End();
  var cb = encoder.Finish();
  device.GetQueue().Submit(cb);
  swapChain.Present();

  do {
    handler.Handle(System.GetNextEvent());
  } while (System.HasPendingEvents());
}
