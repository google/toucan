using Vertex = float<4>;
Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);
auto verts = new Vertex[3];
verts[0] = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1] = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2] = float<4>( 1.0, -1.0, 0.0, 1.0);
auto vb = new vertex Buffer<Vertex[]>(device, verts);
class Pipeline {
  void vertexShader(VertexBuiltins vb, Vertex v) vertex { vb.position = v; }
  static float<4> green() { return float<4>(0.0, 1.0, 0.0, 1.0); }
  float<4> fragmentShader(FragmentBuiltins fb) fragment { return Pipeline.green(); ; }
}

RenderPipeline* pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
auto framebuffer = swapChain.GetCurrentTexture();
auto encoder = new CommandEncoder(device);
auto renderPass = new RenderPass(encoder, framebuffer);
renderPass.SetPipeline(pipeline);
renderPass.SetVertexBuffer(0, vb);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
return 0.0;
