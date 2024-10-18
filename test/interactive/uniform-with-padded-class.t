using Vertex = float<4>;
var device = new Device();
var window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts : Vertex[3] = { { 0.0, 1.0, 0.0, 1.0 }, {-1.0, -1.0, 0.0, 1.0 }, { 1.0, -1.0, 0.0, 1.0 } };
var vb = new vertex Buffer<Vertex[]>(device, &verts);
class Padding {
  var pad1 : float;
}

class Uniforms {
  var padding : Padding;
  var intPadding : int;
  var color : float<4>;
}
class Bindings {
  var uniforms : uniform Buffer<Uniforms>*;
}
class Pipeline {
  vertex main(vb : VertexBuiltins^) { vb.position = vertices.Get(); }
  fragment main(fb : FragmentBuiltins^) {
    fragColor.Set(bindings.Get().uniforms.MapReadUniform().color);
  }
  var vertices : vertex Buffer<Vertex[]>*;
  var fragColor : ColorAttachment<PreferredSwapChainFormat>*;
  var bindings : BindGroup<Bindings>*;
}
var uniforms = new uniform Buffer<Uniforms>(device, { color = { 0.0, 1.0, 0.0, 1.0 } });
var bg = new BindGroup<Bindings>(device, { uniforms });
var stagingBuffer = new writeonly Buffer<Uniforms>(device);
var pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
var framebuffer = swapChain.GetCurrentTexture();
var encoder = new CommandEncoder(device);
var fb = new ColorAttachment<PreferredSwapChainFormat>(framebuffer, Clear, Store);
var renderPass = new RenderPass<Pipeline>(encoder,
  {vertices = vb, fragColor = fb, bindings = bg }
);
renderPass.SetPipeline(pipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
