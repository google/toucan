using Vertex = float<4>;
Device* device = new Device();
Window* window = new Window({0, 0}, System.GetScreenSize());
auto swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
Vertex[3] verts = { { 0.0, 1.0, 0.0, 1.0 }, {-1.0, -1.0, 0.0, 1.0 }, { 1.0, -1.0, 0.0, 1.0 } };
auto vb = new vertex Buffer<Vertex[]>(device, &verts);
class Pipeline {
  static deviceonly Vertex helper(vertex Buffer<Vertex[]>* v) { return v.Get(); }
  void vertexShader(VertexBuiltins^ vb) vertex { vb.position = Pipeline.helper(vertices); }
  void fragmentShader(FragmentBuiltins^ fb) fragment { fragColor.Set( {0.0, 1.0, 0.0, 1.0} ); }
  vertex Buffer<Vertex[]>*                   vertices;
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
}
auto pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
auto encoder = new CommandEncoder(device);
auto fb = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
auto renderPass = new RenderPass<Pipeline>(encoder, { vertices = vb, fragColor = fb });
renderPass.SetPipeline(pipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
