class Vertex {
  float<4> position;
  float<4> color;
};
using Varyings = float<4>;
Device* device = new Device();
Window* window = new Window({0, 0}, {640, 480});
Queue* queue = device.GetQueue();
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = new Vertex[4];
verts[0].position = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[1].position = float<4>( 1.0, -1.0, 0.0, 1.0);
verts[2].position = float<4>(-1.0,  1.0, 0.0, 1.0);
verts[3].position = float<4>( 1.0,  1.0, 0.0, 1.0);
verts[0].color = float<4>(1.0, 1.0, 1.0, 1.0);
verts[1].color = float<4>(1.0, 0.0, 0.0, 1.0);
verts[2].color = float<4>(0.0, 1.0, 0.0, 1.0);
verts[3].color = float<4>(0.0, 0.0, 1.0, 1.0);
var indices = new ushort[6];
indices[0] = 0s;
indices[1] = 1s;
indices[2] = 2s;
indices[3] = 1s;
indices[4] = 2s;
indices[5] = 3s;
var vb = new vertex Buffer<Vertex[]>(device, verts);
var ib = new index Buffer<ushort[]>(device, indices);
class Pipeline {
  Varyings vertexShader(VertexBuiltins^ vb) vertex {
    var v = vertices.Get();
    vb.position = v.position;
    return v.color;
  }
  void fragmentShader(FragmentBuiltins^ fb, Varyings v) fragment { fragColor.Set(v); }
  vertex Buffer<Vertex[]>* vertices;
  index Buffer<ushort[]>* indices;
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
}
var pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
var encoder = new CommandEncoder(device);
var fb = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
var renderPass = new RenderPass<Pipeline>(encoder, { vertices = vb, indices = ib, fragColor = fb });
renderPass.SetPipeline(pipeline);
renderPass.DrawIndexed(6, 1, 0, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
