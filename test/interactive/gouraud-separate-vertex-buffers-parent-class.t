var device = new Device();
var window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);

var positions : [3]float<2> = {
  { 0.0,  1.0 },
  {-1.0, -1.0 },
  { 1.0, -1.0 }
};

var colors : [3]float<3> = {
  {1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0},
  {0.0, 0.0, 1.0}
};

class BasePipeline {
  var position : *VertexInput<float<2>>;
}

class Pipeline : BasePipeline {
  vertex main(vb : &VertexBuiltins) : float<3> {
    var v = position.Get();
    vb.position = float<4>(v.x, v.y, 0.0, 1.0);
    return color.Get();
  }
  fragment main(fb : &FragmentBuiltins, varyings : float<3>) {
    fragColor.Set(float<4>(varyings.r, varyings.g, varyings.b, 1.0));
  }
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
  var color : *VertexInput<float<3>>;
}

var pipeline = new RenderPipeline<Pipeline>(device);
var encoder = new CommandEncoder(device);
var positionsVB = new vertex Buffer<[]float<2>>(device, &positions);
var colorsVB = new vertex Buffer<[]float<3>>(device, &colors);
var p : Pipeline;
p.fragColor = swapChain.GetCurrentTexture().CreateColorAttachment(LoadOp.Clear);
p.position = new VertexInput<float<2>>(positionsVB);
p.color = new VertexInput<float<3>>(colorsVB);
var renderPass = new RenderPass<Pipeline>(encoder, &p);
renderPass.SetPipeline(pipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
