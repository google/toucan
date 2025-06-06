using Vertex = float<4>;

class ComputeBindings {
  var vertStorage : *storage Buffer<[]Vertex>;
}

class BumpCompute {
  compute(1, 1, 1) main(cb : &ComputeBuiltins) {
    var verts = bindings.Get().vertStorage.Map();
    var pos = cb.globalInvocationId.x;
    verts[pos] += float<4>( 0.005,  0.0, 0.0, 0.0);
  }
  var bindings : *BindGroup<ComputeBindings>;
}

var device = new Device();
var window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = [3] new Vertex;
verts[0] = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1] = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2] = float<4>( 1.0, -1.0, 0.0, 1.0);
var vb = new vertex storage Buffer<[]Vertex>(device, verts);
class Pipeline {
  vertex main(vb : &VertexBuiltins) { vb.position = vert.Get(); }
  fragment main(fb : &FragmentBuiltins) { fragColor.Set(float<4>(0.0, 1.0, 0.0, 1.0)); }
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
  var vert : *VertexInput<Vertex>;
}

var pipeline = new RenderPipeline<Pipeline>(device);
var computePipeline = new ComputePipeline<BumpCompute>(device);

var cb : ComputeBindings;
cb.vertStorage = vb;
var storageBG = new BindGroup<ComputeBindings>(device, &cb);

while (System.IsRunning()) {
  var encoder = new CommandEncoder(device);
  var computePass = new ComputePass<BumpCompute>(encoder, {bindings = storageBG});
  computePass.SetPipeline(computePipeline);
  computePass.Dispatch(verts.length, 1, 1);
  computePass.End();
  var p : Pipeline;
  p.vert = new VertexInput<Vertex>(vb);
  p.fragColor = swapChain.GetCurrentTexture().CreateColorAttachment(LoadOp.Clear);
  var renderPass = new RenderPass<Pipeline>(encoder, &p);
  renderPass.SetPipeline(pipeline);
  renderPass.Draw(3, 1, 0, 0);
  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();

  while (System.HasPendingEvents()) System.GetNextEvent();
}
