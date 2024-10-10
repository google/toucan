class Vertex {
  var position : float<4>;
  var color : float<4>;
};
using Varyings = float<4>;
var device = new Device();
var window = new Window({0, 0}, {640, 480});
var queue = device.GetQueue();
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
var indices = new uint[6];
indices[0] = 0;
indices[1] = 1;
indices[2] = 2;
indices[3] = 1;
indices[4] = 2;
indices[5] = 3;
var vb = new vertex Buffer<Vertex[]>(device, verts);
var ib = new index Buffer<uint[]>(device, indices);
class Pipeline {
  vertexShader(VertexBuiltins^ vb) vertex : Varyings {
    var v = vertices.Get();
    vb.position = v.position;
    return v.color;
  }
  fragmentShader(FragmentBuiltins^ fb, Varyings v) fragment { fragColor.Set(v); }
  var vertices : vertex Buffer<Vertex[]>*;
  var indices : index Buffer<uint[]>*;
  var fragColor : ColorAttachment<PreferredSwapChainFormat>*;
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
