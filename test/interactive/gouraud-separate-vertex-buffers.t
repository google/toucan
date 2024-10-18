var device = new Device();
var window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);

var positions : float<2>[3];
positions[0] = { 0.0,  1.0};
positions[1] = {-1.0, -1.0};
positions[2] = { 1.0, -1.0};

var colors : float<3>[3];
colors[0] = {1.0, 0.0, 0.0};
colors[1] = {0.0, 1.0, 0.0};
colors[2] = {0.0, 0.0, 1.0};

class Pipeline {
  vertex main(vb : VertexBuiltins^) : float<3> {
    var v = position.Get();
    vb.position = float<4>(v.x, v.y, 0.0, 1.0);
    return color.Get();
  }
  fragment main(fb : FragmentBuiltins^, varyings : float<3>) {
    fragColor.Set(float<4>(varyings.r, varyings.g, varyings.b, 1.0));
  }
  var fragColor : ColorAttachment<PreferredSwapChainFormat>*;
  var position : vertex Buffer<float<2>[]>*;
  var color : vertex Buffer<float<3>[]>*;
}

var pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
var encoder = new CommandEncoder(device);
var p : Pipeline;
p.fragColor = swapChain.GetCurrentTexture().CreateColorAttachment(Clear, Store);
p.position = new vertex Buffer<float<2>[]>(device, &positions);
p.color = new vertex Buffer<float<3>[]>(device, &colors);
var renderPass = new RenderPass<Pipeline>(encoder, &p);
renderPass.SetPipeline(pipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
