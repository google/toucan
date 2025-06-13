using Vertex = float<2>;

class ComputeBindings {
  var vertStorage : *storage Buffer<[]Vertex>;
}

class BumpCompute {
  compute(1, 1, 1) main(cb : &ComputeBuiltins) {
    var verts = bindings.Get().vertStorage.Map();
    var pos = cb.globalInvocationId.x;
    verts[pos] += float<2>{0.005,  0.0};
  }
  var bindings : *BindGroup<ComputeBindings>;
}

class Pipeline {
  vertex main(vb : &VertexBuiltins) { vb.position = {@vert.Get(), 0.0, 1.0}; }
  fragment main(fb : &FragmentBuiltins) { fragColor.Set(float<4>(0.0, 1.0, 0.0, 1.0)); }
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
  var vert : *VertexInput<Vertex>;
}

var device = new Device();
var window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = [3]Vertex{ { 0.0, 1.0 }, {-1.0, -1.0 }, { 1.0, -1.0 } };
var vb = new vertex storage Buffer<[]Vertex>(device, &verts);

var pipeline = new RenderPipeline<Pipeline>(device);
var computePipeline = new ComputePipeline<BumpCompute>(device);
var storageBG = new BindGroup<ComputeBindings>(device, { vb });

while (System.IsRunning()) {
  var encoder = new CommandEncoder(device);
  var computePass = new ComputePass<BumpCompute>(encoder, {bindings = storageBG});
  computePass.SetPipeline(computePipeline);
  computePass.Dispatch(verts.length, 1, 1);
  computePass.End();
  var fb = swapChain.GetCurrentTexture().CreateColorAttachment(LoadOp.Clear);
  var vi = new VertexInput<Vertex>(vb);
  var renderPass = new RenderPass<Pipeline>(encoder, {vert = vi, fragColor = fb});
  renderPass.SetPipeline(pipeline);
  renderPass.Draw(3, 1, 0, 0);
  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();

  while (System.HasPendingEvents()) System.GetNextEvent();
}
