class Vertex {
  var position : float<2>;
  var color : float<3>;
}

class Pipeline {
  vertex main(vb : &VertexBuiltins) : float<3> {
    var v = vertices.Get();
    vb.position = {@v.position, 0.0, 1.0};
    return v.color;
  }
  fragment main(fb : &FragmentBuiltins, varyings : float<3>) {
    fragColor.Set({@varyings, 1.0});
  }
  var fragColor : *ColorOutput<PreferredPixelFormat>;
  var vertices : *VertexInput<Vertex>;
}

var device = new Device();
var window = new Window({640, 480});
var swapChain = new SwapChain<PreferredPixelFormat>(device, window);
var verts = [3]Vertex{
  { position = { 0.0,  1.0 }, color = { 1.0, 0.0, 0.0 } },
  { position = {-1.0, -1.0 }, color = { 0.0, 1.0, 0.0 } },
  { position = { 1.0, -1.0 }, color = { 0.0, 0.0, 1.0 } }
};

var vb = new vertex Buffer<[]Vertex>(device, &verts);
var vi = new VertexInput<Vertex>(vb);
var fb = swapChain.GetCurrentTexture().CreateColorOutput(LoadOp.Clear);
var pipeline = new RenderPipeline<Pipeline>(device);
var encoder = new CommandEncoder(device);
var renderPass = new RenderPass<Pipeline>(encoder, { vertices = vi, fragColor = fb });
renderPass.SetPipeline(pipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
