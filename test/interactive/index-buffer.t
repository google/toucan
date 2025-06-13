class Vertex {
  var position : float<2>;
  var color : float<4>;
};

class Pipeline {
  vertex main(vb : &VertexBuiltins) : float<4> {
    var v = vertices.Get();
    vb.position = {@v.position, 0.0, 1.0};
    return v.color;
  }
  fragment main(fb : &FragmentBuiltins, v : float<4>) { fragColor.Set(v); }
  var vertices : *VertexInput<Vertex>;
  var indices : *index Buffer<[]uint>;
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
}

var device = new Device();
var window = new Window({0, 0}, {640, 480});
var queue = device.GetQueue();
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = [4]Vertex{
 { position = {-1.0, -1.0}, color = {1.0, 1.0, 1.0, 1.0} },
 { position = { 1.0, -1.0}, color = {1.0, 0.0, 0.0, 1.0} },
 { position = {-1.0,  1.0}, color = {0.0, 1.0, 0.0, 1.0} },
 { position = { 1.0,  1.0}, color = {0.0, 0.0, 1.0, 1.0} }
};
var indices = [6]uint{ 0, 1, 2, 1, 2, 3 };
var vb = new vertex Buffer<[]Vertex>(device, &verts);
var ib = new index Buffer<[]uint>(device, &indices);
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
