class Vertex {
  var position : float<4>;
  var color : float<4>;
};
using Varyings = float<4>;
var device = new Device();
var window = new Window({0, 0}, {640, 480});
var queue = device.GetQueue();
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = [4] new Vertex;
verts[0].position = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[1].position = float<4>( 1.0, -1.0, 0.0, 1.0);
verts[2].position = float<4>(-1.0,  1.0, 0.0, 1.0);
verts[3].position = float<4>( 1.0,  1.0, 0.0, 1.0);
verts[0].color = float<4>(1.0, 1.0, 1.0, 1.0);
verts[1].color = float<4>(1.0, 0.0, 0.0, 1.0);
verts[2].color = float<4>(0.0, 1.0, 0.0, 1.0);
verts[3].color = float<4>(0.0, 0.0, 1.0, 1.0);
var indices = [6] new ushort;
indices[0] = 0s;
indices[1] = 1s;
indices[2] = 2s;
indices[3] = 1s;
indices[4] = 2s;
indices[5] = 3s;
var vb = new vertex Buffer<[]Vertex>(device, verts);
var ib = new index Buffer<[]ushort>(device, indices);
class Pipeline {
  vertex main(vb : &VertexBuiltins) : Varyings {
    var v = vertices.Get();
    vb.position = v.position;
    return v.color;
  }
  fragment main(fb : &FragmentBuiltins, v : Varyings) { fragColor.Set(v); }
  var vertices : *VertexInput<Vertex>;
  var indices : *index Buffer<[]ushort>;
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
}
var pipeline = new RenderPipeline<Pipeline>(device);
var encoder = new CommandEncoder(device);
var vi = new VertexInput<Vertex>(vb);
var fb = swapChain.GetCurrentTexture().CreateColorAttachment(LoadOp.Clear);
var renderPass = new RenderPass<Pipeline>(encoder, { vertices = vi, indices = ib, fragColor = fb });
renderPass.SetPipeline(pipeline);
renderPass.DrawIndexed(6, 1, 0, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
