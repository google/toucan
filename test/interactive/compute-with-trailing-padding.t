class Vertex {
  var position : float<4>;
  var pad : float;
}

class ComputeBindings {
  var vertStorage : storage Buffer<Vertex[]>*;
}

class BumpCompute {
  computeShader(ComputeBuiltins^ cb) compute(1, 1, 1) {
    var verts = bindings.Get().vertStorage.MapReadWriteStorage();
    var pos = cb.globalInvocationId.x;
    verts[pos].position += float<4>( 0.005,  0.0, 0.0, 0.0);
  }
  var bindings : BindGroup<ComputeBindings>*;
}

var device = new Device();
var window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = new Vertex[3];
verts[0].position = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1].position = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2].position = float<4>( 1.0, -1.0, 0.0, 1.0);
var vb = new vertex storage Buffer<Vertex[]>(device, verts);
class Pipeline {
  vertexShader(VertexBuiltins^ vb) vertex { var v = vert.Get(); vb.position = v.position; }
  fragmentShader(FragmentBuiltins^ fb) fragment { fragColor.Set(float<4>(0.0, 1.0, 0.0, 1.0)); }
  var fragColor : ColorAttachment<PreferredSwapChainFormat>*;
  var vert : vertex Buffer<Vertex[]>*;
}

var pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
var computePipeline = new ComputePipeline<BumpCompute>(device);

var cb : ComputeBindings;
cb.vertStorage = vb;
var storageBG = new BindGroup<ComputeBindings>(device, &cb);

while (System.IsRunning()) {
  var encoder = new CommandEncoder(device);
  var bc : BumpCompute;
  bc.bindings = storageBG;
  var computePass = new ComputePass<BumpCompute>(encoder, &bc);
  computePass.SetPipeline(computePipeline);
  computePass.Dispatch(verts.length, 1, 1);
  computePass.End();
  var p : Pipeline;
  p.vert = vb;
  p.fragColor = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
  var renderPass = new RenderPass<Pipeline>(encoder, &p);
  renderPass.SetPipeline(pipeline);
  renderPass.Draw(3, 1, 0, 0);
  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();

  while (System.HasPendingEvents()) System.GetNextEvent();
}
