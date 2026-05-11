#include "api.t"

using Vertex = float<2>;
class Pipeline {
  deviceonly vhelper() : Vertex { return vertices.Get(); }
  deviceonly fhelper() { fragColor.Set( {0.0, 1.0, 0.0, 1.0} ); }
  vertex main(vb : &VertexBuiltins) { vb.position = {@this.vhelper(), 0.0, 1.0}; }
  fragment main(fb : &FragmentBuiltins) { this.fhelper(); }
  var vertices : *VertexInput<Vertex>;
  var fragColor : *ColorOutput<PreferredPixelFormat>;
}
var device = new Device();
var window = new Window({640, 480});
var swapChain = new SwapChain<PreferredPixelFormat>(device, window);
var verts : [3]Vertex = { {0.0, 1.0}, {-1.0, -1.0}, {1.0, -1.0} };
var vb = new vertex Buffer<[]Vertex>(device, &verts);
var pipeline = new RenderPipeline<Pipeline>(device);
var encoder = new CommandEncoder(device);
var vi = new VertexInput<Vertex>(vb);
var fb = swapChain.GetCurrentTexture().CreateColorOutput(LoadOp.Clear);
var renderPass = new RenderPass<Pipeline>(encoder, { vertices = vi, fragColor = fb });
renderPass.SetPipeline(pipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
