using Vertex = float<2>;

class Pipeline {
  vertex main(vb : &VertexBuiltins) { vb.position = {@position.Get(), 0.0, 1.0}; }
  static green() : float<4> { return float<4>(0.0, 1.0, 0.0, 1.0); }
  fragment main(fb : &FragmentBuiltins) { fragColor.Set(Pipeline.green()); }
  var position : *VertexInput<Vertex>;
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
}

var device = new Device();
var window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = [3]Vertex{ { 0.0, 1.0 }, {-1.0, -1.0}, { 1.0, -1.0 } };
var vb = new vertex Buffer<[]Vertex>(device, &verts);
var vi = new VertexInput<Vertex>(vb);

var pipeline = new RenderPipeline<Pipeline>(device);
var encoder = new CommandEncoder(device);
var fb = swapChain.GetCurrentTexture().CreateColorAttachment(LoadOp.Clear);
var renderPass = new RenderPass<Pipeline>(encoder, { position = vi, fragColor = fb });
renderPass.SetPipeline(pipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
