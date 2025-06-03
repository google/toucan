class Pipeline {
  vertex main(vb : &VertexBuiltins) { vb.position = {@vertices.Get(), 0.0, 1.0}; }
  fragment main(fb : &FragmentBuiltins) { fragColor.Set( {0.0, 1.0, 0.0, 1.0} ); }
  var vertices : *VertexInput<float<2>>;
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
}
var device = new Device();
var window = new Window(System.GetScreenSize());
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = [3]float<2>{ { 0.0, 1.0 }, {-1.0, -1.0 }, { 1.0, -1.0 } };
var vb = new vertex Buffer<[]float<2>>(device, &verts);
var vi = new VertexInput<float<2>>(vb);
var fb = swapChain.GetCurrentTexture().CreateColorAttachment(LoadOp.Clear);
var pipeline = new RenderPipeline<Pipeline>(device);
var encoder = new CommandEncoder(device);
var renderPass = new RenderPass<Pipeline>(encoder, { vertices = vi, fragColor = fb });
renderPass.SetPipeline(pipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
