class Vertex {
  var position : float<4>;
  var pad : float;
}

class ComputeBindings {
  var vertStorage : *storage Buffer<[]Vertex>;
}

class BumpCompute {
  compute(1, 1, 1) main(cb : &ComputeBuiltins) {
    var verts = bindings.Get().vertStorage.Map();
    var pos = cb.globalInvocationId.x;
    verts[pos].position += float<4>( 0.005,  0.0, 0.0, 0.0);
  }
  var bindings : *BindGroup<ComputeBindings>;
}

var device = new Device();
var window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = [3] new Vertex;
verts[0].position = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1].position = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2].position = float<4>( 1.0, -1.0, 0.0, 1.0);
var vb = new vertex storage Buffer<[]Vertex>(device, verts);
class Pipeline {
  vertex main(vb : &VertexBuiltins) { var v = vert.Get(); vb.position = v.position; }
  fragment main(fb : &FragmentBuiltins) { fragColor.Set(float<4>(0.0, 1.0, 0.0, 1.0)); }
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
  var vert : *vertex Buffer<[]Vertex>;
}

var pipeline = new RenderPipeline<Pipeline>(device, {}, TriangleList);
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
  var fb = swapChain.GetCurrentTexture().CreateColorAttachment(Clear, Store);
  var renderPass = new RenderPass<Pipeline>(encoder, {vert = vb, fragColor = fb});
  renderPass.SetPipeline(pipeline);
  renderPass.Draw(3, 1, 0, 0);
  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();

  while (System.HasPendingEvents()) System.GetNextEvent();
}
