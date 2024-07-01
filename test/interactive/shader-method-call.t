using Vertex = float<4>;
Device* device = new Device();
Window* window = new Window(0, 0, 640, 480);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
auto verts = new Vertex[3];
verts[0] = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1] = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2] = float<4>( 1.0, -1.0, 0.0, 1.0);
auto vb = new vertex Buffer<Vertex[]>(device, verts);
class Pipeline {
  void vertexShader(VertexBuiltins vb) vertex { vb.position = position.Get(); }
  static float<4> green() { return float<4>(0.0, 1.0, 0.0, 1.0); }
  void fragmentShader(FragmentBuiltins fb) fragment { fragColor.Set(Pipeline.green()); }
  vertex Buffer<Vertex[]>* position;
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
}

auto pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
auto encoder = new CommandEncoder(device);
auto fb = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
auto renderPass = new RenderPass<Pipeline>(encoder, { position = vb, fragColor = fb });
renderPass.SetPipeline(pipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
