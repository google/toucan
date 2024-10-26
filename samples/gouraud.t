class Vertex {
  var position : float<4>;
  var color : float<3>;
}

var device = new Device();
var window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts : [3]Vertex = {
  { position = { 0.0,  1.0, 0.0, 1.0 }, color = { 1.0, 0.0, 0.0 } },
  { position = {-1.0, -1.0, 0.0, 1.0 }, color = { 0.0, 1.0, 0.0 } },
  { position = { 1.0, -1.0, 0.0, 1.0 }, color = { 0.0, 0.0, 1.0 } }
};

var vb = new vertex Buffer<[]Vertex>(device, &verts);
class Pipeline {
  vertex main(vb : ^VertexBuiltins) : float<3> {
    var v = vertices.Get();
    vb.position = v.position;
    return v.color;
  }
  fragment main(fb : ^FragmentBuiltins, varyings : float<3>) {
    fragColor.Set(float<4>(varyings.r, varyings.g, varyings.b, 1.0));
  }
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
  var vertices : *vertex Buffer<[]Vertex>;
}
var pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
var encoder = new CommandEncoder(device);
var fb = swapChain.GetCurrentTexture().CreateColorAttachment(Clear, Store);
var renderPass = new RenderPass<Pipeline>(encoder, { vertices = vb, fragColor = fb });
renderPass.SetPipeline(pipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
