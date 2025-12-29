using Vertex = float<2>;
class Uniforms {
  var color : float<4>;
}
class Bindings {
  var uniforms : *uniform Buffer<Uniforms>;
}
class Pipeline {
  vertex main(vb : &VertexBuiltins) { vb.position = {@vertices.Get(), 0.0, 1.0}; }
  fragment main(fb : &FragmentBuiltins) {
    fragColor.Set(bindings.Get().uniforms.MapRead().color);
  }
  var vertices : *VertexInput<Vertex>;
  var fragColor : *ColorOutput<PreferredPixelFormat>;
  var bindings : *BindGroup<Bindings>;
}
var device = new Device();
var window = new Window({640, 480});
var swapChain = new SwapChain<PreferredPixelFormat>(device, window);
var verts : [3]Vertex = { { 0.0, 1.0 }, {-1.0, -1.0 }, { 1.0, -1.0 } };
var vb = new vertex Buffer<[]Vertex>(device, &verts);
var uniformBuffer = new uniform Buffer<Uniforms>(device);
var bg = new BindGroup<Bindings>(device, { uniformBuffer });
var stagingBuffer = new hostwriteable Buffer<Uniforms>(device);
var pipeline = new RenderPipeline<Pipeline>(device);
for (var i = 0; i < 300 && System.IsRunning(); ++i) {
  var encoder = new CommandEncoder(device);
  var f = i as float / 300.0;
  stagingBuffer.MapWrite().color = float<4>(1.0 - f, f, 0.0, 1.0);
  uniformBuffer.CopyFromBuffer(encoder, stagingBuffer);
  var vi = new VertexInput<Vertex>(vb);
  var fb = swapChain.GetCurrentTexture().CreateColorOutput(LoadOp.Clear);
  var renderPass = new RenderPass<Pipeline>(encoder, { vertices = vi, fragColor = fb, bindings = bg });
  renderPass.SetPipeline(pipeline);
  renderPass.Draw(3, 1, 0, 0);
  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();
  while (System.HasPendingEvents()) {
    System.GetNextEvent();
  }
}
