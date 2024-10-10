using Vertex = float<4>;
var device = new Device();
var window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = new Vertex[3];
verts[0] = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1] = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2] = float<4>( 1.0, -1.0, 0.0, 1.0);
var vb = new vertex Buffer<Vertex[]>(device, verts);
class Pipeline {
  vertexShader(VertexBuiltins^ vb) vertex { vb.position = position.Get(); }
  static green() : float<4> { return float<4>(0.0, 1.0, 0.0, 1.0); }
  fragmentShader(FragmentBuiltins^ fb) fragment { fragColor.Set(Pipeline.green()); }
  var position : vertex Buffer<Vertex[]>*;
  var fragColor : ColorAttachment<PreferredSwapChainFormat>*;
}

var pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
var encoder = new CommandEncoder(device);
var fb = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
var renderPass = new RenderPass<Pipeline>(encoder, { position = vb, fragColor = fb });
renderPass.SetPipeline(pipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
