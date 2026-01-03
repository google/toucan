include "cube.t"
include "cube-loader.t"
include "cubic.t"
include "event-handler.t"
include "mipmap-generator.t"
include "quaternion.t"
include "transform.t"
include "teapot.t"

class Vertex {
  var position : float<3>;
  var normal : float<3>;
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
      result.normal = Math.normalize(Math.cross(vTangent, uTangent));
    }
    return result;
  }
  var uCubics : [4]Cubic<float<3>>;
  var vCubics : [4]Cubic<float<3>>;
}

var level = 64;
var patchWidth = level + 1;
var numPatches = teapotControlIndices.length / 16;
var tessTeapotIndices = [numPatches * level * level * 6] new uint;

var vi = 0, ii = 0;
for (var k = 0; k < numPatches; ++k) {
  for (var i = 0; i < level; ++i) {
    for (var j = 0; j < level; ++j) {
      tessTeapotIndices[ii++] = vi;
      tessTeapotIndices[ii++] = vi + 1;
      tessTeapotIndices[ii++] = vi + patchWidth + 1;
      tessTeapotIndices[ii++] = vi;
      tessTeapotIndices[ii++] = vi + patchWidth + 1;
      tessTeapotIndices[ii++] = vi + patchWidth;
      ++vi;
    }
    ++vi;           // skip the last column
  }
  vi += patchWidth; // skip the last row
}

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

class ComputeUniforms {
  var patchWidth : uint;
  var scale : float;
}

class ComputeBindings {
  var controlPoints : *storage Buffer<[]float<3>>;
  var controlIndices : *storage Buffer<[]uint>;
  var vertices : *storage Buffer<[]Vertex>;
  var uniforms : *uniform Buffer<ComputeUniforms>;
}

class BicubicComputePipeline {
  compute(8, 8, 1) main(cb : &ComputeBuiltins) {
    var controlPoints = bindings.Get().controlPoints.Map();
    var controlIndices = bindings.Get().controlIndices.Map();
    var vertices = bindings.Get().vertices.Map();
    var uniforms = bindings.Get().uniforms.MapRead();
    var u = (float) cb.globalInvocationId.x * uniforms.scale;
    var v = (float) cb.globalInvocationId.y * uniforms.scale;
    if (u > 1.0 || v > 1.0) {
      return;
    }
    var k = cb.globalInvocationId.z * 16u;
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
    var id = cb.globalInvocationId.x + uniforms.patchWidth * (cb.globalInvocationId.y + uniforms.patchWidth * cb.globalInvocationId.z);
    vertices[id] = patch.Evaluate(u, v);
  }
  var bindings : *BindGroup<ComputeBindings>;
}

class SkyboxPipeline : DrawPipeline {
    vertex main(vb : &VertexBuiltins) : float<3> {
        var v = position.Get();
        var uniforms = bindings.Get().uniforms.MapRead();
        var pos = float<4>(@v, 1.0);
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
        var pos = viewModel * float<4>(@v.position, 1.0);
        var normal = viewModel * float<4>(@n, 0.0);
        vb.position = uniforms.projection * pos;
        var varyings : Vertex;
        varyings.position = pos.xyz;
        varyings.normal = normal.xyz;
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

var tessPipeline = new ComputePipeline<BicubicComputePipeline>(device);

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

var teapotPipeline = new RenderPipeline<ReflectionPipeline>(device);
var teapotBindings = Bindings{
  sampler = cubeBindings.sampler,
  textureView = cubeBindings.textureView,
  uniforms = new uniform Buffer<Uniforms>(device)
};
var numVerticesPerPatch = patchWidth * patchWidth;
var teapotVB = new vertex storage Buffer<[]Vertex>(device, numPatches * numVerticesPerPatch);

var teapotControlPointsBuffer = new storage Buffer<[]float<3>>(device, teapotControlPoints.length);
var computeBindings = new BindGroup<ComputeBindings>(device, {
  controlPoints = teapotControlPointsBuffer,
  controlIndices = new storage Buffer<[]uint>(device, &teapotControlIndices),
  vertices = teapotVB,
  uniforms = new uniform Buffer<ComputeUniforms>(device, { patchWidth = patchWidth, scale = 1.0 / (float) level } )
});

var teapotData = ReflectionPipeline{
  vert = new VertexInput<Vertex>(teapotVB),
  indexBuffer = new index Buffer<[]uint>(device, tessTeapotIndices),
  bindings = new BindGroup<Bindings>(device, &teapotBindings)
};

var handler = EventHandler{ distance = 10.0 };
var teapotQuat = Quaternion(float<3>(1.0, 0.0, 0.0), -Math.pi / 2.0);
teapotQuat.normalize();
var teapotRotation = teapotQuat.toMatrix();
var depthBuffer = new renderable Texture2D<Depth24Plus>(device, window.GetSize());
var uniforms : Uniforms;
var prevWindowSize = uint<2>{0, 0};
var startTime = System.GetCurrentTime();
var animCurves : [4]Cubic<float>;
animCurves[0].FromBezier({1.0, 1.0, 1.5, 1.5});
animCurves[1].FromBezier({1.5, 1.5, 1.0, 1.0});
animCurves[2].FromBezier({1.0, 1.0, 0.5, 0.5});
animCurves[3].FromBezier({0.5, 0.5, 1.0, 1.0});
var keyTimes : [4]float = { 0.0, 0.5, 1.5, 1.7 };
var duration = 2.0;
var animTeapotControlPoints = [teapotControlPoints.length] new float<3>;
while (System.IsRunning()) {
  var animTime = (float) ((System.GetCurrentTime() - startTime) % duration);
  var key = keyTimes.length - 1;
  var keyEnd = duration;
  for (var i = 0; i < keyTimes.length - 1; ++i) {
    if (animTime >= keyTimes[i] && animTime < keyTimes[i + 1]) {
      key = i;
      keyEnd = keyTimes[i + 1];
    }
  }
  var keyStart = keyTimes[key];
  var t = animCurves[key].Evaluate((animTime - keyStart) / (keyEnd - keyStart));

  for (var i = 0; i < teapotControlPoints.length; ++i) {
    animTeapotControlPoints[i] = teapotControlPoints[i];
  }

  for (var i = 0; i < teapotControlIndices.length; i += 16) {
    animTeapotControlPoints[teapotControlIndices[i + 5]] *= t;
    animTeapotControlPoints[teapotControlIndices[i + 6]] *= t;
    animTeapotControlPoints[teapotControlIndices[i + 9]] *= t;
    animTeapotControlPoints[teapotControlIndices[i + 10]] *= t;
  }

  teapotControlPointsBuffer.SetData(animTeapotControlPoints);
  var orientation = Quaternion(float<3>(0.0, 1.0, 0.0), handler.rotation.x);
  orientation = orientation.mul(Quaternion(float<3>(1.0, 0.0, 0.0), handler.rotation.y));
  orientation.normalize();
  var newSize = window.GetSize();
  if (Math.any(newSize != prevWindowSize)) {
    swapChain.Resize(newSize);
    depthBuffer = new renderable Texture2D<Depth24Plus>(device, newSize);
    var aspectRatio = (float) newSize.x / (float) newSize.y;
    uniforms.projection = Transform.projection(0.5, 200.0, -aspectRatio, aspectRatio, -1.0, 1.0);
    prevWindowSize = newSize;
  }
  uniforms.view = Transform.translation({0.0, 0.0, -handler.distance});
  uniforms.view *= orientation.toMatrix();
  uniforms.model = Transform.scale({100.0, 100.0, 100.0});
  uniforms.viewInverse = Transform.invert(uniforms.view);
  cubeBindings.uniforms.SetData(&uniforms);
  uniforms.model = teapotRotation * Transform.scale({2.0, 2.0, 2.0});
  teapotBindings.uniforms.SetData(&uniforms);
  var encoder = new CommandEncoder(device);

  var tessPass = new ComputePass<BicubicComputePipeline>(encoder, { bindings = computeBindings });
  tessPass.SetPipeline(tessPipeline);
  tessPass.Dispatch((patchWidth + 7) / 8, (patchWidth + 7) / 8, numPatches);
  tessPass.End();

  var fb = swapChain.GetCurrentTexture().CreateColorOutput(LoadOp.Clear);
  var db = depthBuffer.CreateDepthStencilOutput(LoadOp.Clear);
  var renderPass = new RenderPass<DrawPipeline>(encoder, { fragColor = fb, depth = db });

  var cubePass = new RenderPass<SkyboxPipeline>(renderPass);
  cubePass.SetPipeline(cubePipeline);
  cubePass.Set(&cubeData);
  cubePass.DrawIndexed(cubeIndices.length, 1, 0, 0, 0);

  var teapotPass = new RenderPass<ReflectionPipeline>(renderPass);
  teapotPass.SetPipeline(teapotPipeline);
  teapotPass.Set(&teapotData);
  teapotPass.DrawIndexed(tessTeapotIndices.length, 1, 0, 0, 0);

  renderPass.End();
  var cb = encoder.Finish();
  device.GetQueue().Submit(cb);
  swapChain.Present();

  while (System.HasPendingEvents()) {
    handler.Handle(System.GetNextEvent());
  }
}
