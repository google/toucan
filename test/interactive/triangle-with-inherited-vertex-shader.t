using Vertex = float<2>;

class PipelineBase {
  vertex main(vb : &VertexBuiltins) { vb.position = {@vertices.Get(), 0.0, 1.0}; }
  var vertices : *VertexInput<Vertex>;
}

class Pipeline : PipelineBase {
  fragment main(fb : &FragmentBuiltins) { fragColor.Set( {0.0, 1.0, 0.0, 1.0} ); }
  var fragColor : *ColorOutput<PreferredPixelFormat>;
}

var device = new Device();
var window = new Window({640, 480});
var swapChain = new SwapChain<PreferredPixelFormat>(device, window);
var verts : [3]Vertex = { { 0.0, 1.0 }, {-1.0, -1.0 }, { 1.0, -1.0 } };
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
