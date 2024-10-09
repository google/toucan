using Vertex = float<4>;
Device* device = new Device();
Window* window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
Vertex[3] verts = { { 0.0, 1.0, 0.0, 1.0 }, {-1.0, -1.0, 0.0, 1.0 }, { 1.0, -1.0, 0.0, 1.0 } };
var vb = new vertex Buffer<Vertex[]>(device, &verts);
class Uniforms {
  float<4> color;
}
class Bindings {
  uniform Buffer<Uniforms>* uniforms;
}
class Pipeline {
  void vertexShader(VertexBuiltins^ vb) vertex { vb.position = vertices.Get(); }
  void fragmentShader(FragmentBuiltins^ fb) fragment {
    fragColor.Set(bindings.Get().uniforms.MapReadUniform().color);
  }
  vertex Buffer<Vertex[]>* vertices;
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
  BindGroup<Bindings>* bindings;
}
var uniformBuffer = new uniform Buffer<Uniforms>(device);
var bg = new BindGroup<Bindings>(device, { uniformBuffer });
var stagingBuffer = new writeonly Buffer<Uniforms>(device);
var pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
for (int i = 0; i < 1000; ++i) {
  while (System.HasPendingEvents()) {
    System.GetNextEvent();
  }
  var encoder = new CommandEncoder(device);
  writeonly Uniforms^ s = stagingBuffer.MapWrite();
  float f = (float) i / 1000.0;
  s.color = float<4>(1.0 - f, f, 0.0, 1.0);
  stagingBuffer.Unmap();
  encoder.CopyBufferToBuffer(stagingBuffer, uniformBuffer);
  var fb = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
  var renderPass = new RenderPass<Pipeline>(encoder, { vertices = vb, fragColor = fb, bindings = bg });
  renderPass.SetPipeline(pipeline);
  renderPass.Draw(3, 1, 0, 0);
  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();
}
