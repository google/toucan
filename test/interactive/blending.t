using Vertex = float<2>;
class Bindings {
  var color : *uniform Buffer<float<4>>;
}

class Pipeline {
  vertex main(vb : &VertexBuiltins) { vb.position = {@vertices.Get(), 0.0, 1.0}; }
  fragment main(fb : &FragmentBuiltins) { fragColor.Set(bindings.Get().color.MapRead():); }
  var vertices :  *VertexInput<Vertex>;
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
  var bindings :  *BindGroup<Bindings>;
}
var device = new Device();
var window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = [3]Vertex{ { 0.0, 1.0 }, {-1.0, -1.0 }, { 1.0, -1.0 } };
var vb = new vertex Buffer<[]Vertex>(device, &verts);
var encoder = new CommandEncoder(device);
var blendState = BlendState{
  color = { srcFactor = BlendFactor.SrcAlpha, dstFactor = BlendFactor.OneMinusSrcAlpha }
};
var pipeline = new RenderPipeline<Pipeline>(device = device, blendState = &blendState);
var fb = swapChain.GetCurrentTexture().CreateColorAttachment(LoadOp.Clear);
var redUBO = new uniform Buffer<float<4>>(device, { 1.0, 0.0, 0.0, 1.0 });
var redBG = new BindGroup<Bindings>(device, { color = redUBO });
var translucentBlueUBO = new uniform Buffer<float<4>>(device, { 0.0, 0.0, 1.0, 0.5 });
var translucentBlueBG = new BindGroup<Bindings>(device, { color = translucentBlueUBO });
var vi = new VertexInput<Vertex>(vb);
var drawPass = new RenderPass<Pipeline>(encoder, { fragColor = fb, vertices = vi });
drawPass.SetPipeline(pipeline);
drawPass.Set({ bindings = redBG });
drawPass.Draw(3, 1, 0, 0);
drawPass.Set({ bindings = translucentBlueBG });
drawPass.Draw(3, 1, 0, 0);
drawPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
