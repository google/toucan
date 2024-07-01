class Vertex {
  float<4> position;
  float<4> color;
};
using Varyings = float<4>;
Device* device = new Device();
Window* window = new Window(0, 0, 640, 480);
Queue* queue = device.GetQueue();
auto swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
auto verts = new Vertex[4];
verts[0].position = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[1].position = float<4>( 1.0, -1.0, 0.0, 1.0);
verts[2].position = float<4>(-1.0,  1.0, 0.0, 1.0);
verts[3].position = float<4>( 1.0,  1.0, 0.0, 1.0);
verts[0].color = float<4>(1.0, 1.0, 1.0, 1.0);
verts[1].color = float<4>(1.0, 0.0, 0.0, 1.0);
verts[2].color = float<4>(0.0, 1.0, 0.0, 1.0);
verts[3].color = float<4>(0.0, 0.0, 1.0, 1.0);
auto indices = new uint[6];
indices[0] = 0;
indices[1] = 1;
indices[2] = 2;
indices[3] = 1;
indices[4] = 2;
indices[5] = 3;
auto vb = new vertex Buffer<Vertex[]>(device, verts);
auto ib = new index Buffer<uint[]>(device, indices);
class Pipeline {
  Varyings vertexShader(VertexBuiltins vb) vertex {
    auto v = vertices.Get();
    vb.position = v.position;
    return v.color;
  }
  void fragmentShader(FragmentBuiltins fb, Varyings v) fragment { fragColor.Set(v); }
  vertex Buffer<Vertex[]>* vertices;
  index Buffer<uint[]>* indices;
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
}
auto pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
auto encoder = new CommandEncoder(device);
auto fb = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
auto renderPass = new RenderPass<Pipeline>(encoder, { vertices = vb, indices = ib, fragColor = fb });
renderPass.SetPipeline(pipeline);
renderPass.DrawIndexed(6, 1, 0, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
