using Vertex = float<4>;
Device* device = new Device();
Window* window = new Window({0, 0}, System.GetScreenSize());
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
Vertex[3] verts = { { 0.0, 1.0, 0.0, 1.0 }, {-1.0, -1.0, 0.0, 1.0 }, { 1.0, -1.0, 0.0, 1.0 } };
var vb = new vertex Buffer<Vertex[]>(device, &verts);
class Pipeline {
  void vertexShader(VertexBuiltins^ vb) vertex { vb.position = vertices.Get(); }
  void fragmentShader(FragmentBuiltins^ fb) fragment { fragColor.Set( {0.0, 1.0, 0.0, 1.0} ); }
  vertex Buffer<Vertex[]>*                   vertices;
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
}
var pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
var encoder = new CommandEncoder(device);
var fb = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
var renderPass = new RenderPass<Pipeline>(encoder, { vertices = vb, fragColor = fb });
renderPass.SetPipeline(pipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
