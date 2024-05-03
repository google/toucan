using Vertex = float<4>;
Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);
auto verts = new Vertex[3];
verts[0] = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1] = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2] = float<4>( 1.0, -1.0, 0.0, 1.0);
auto vb = new vertex Buffer<Vertex[]>(device, verts);
class Uniforms {
  float<4> color;
}
class Bindings {
  uniform Buffer<Uniforms>* uniforms;
}
class Pipeline {
  void vertexShader(VertexBuiltins vb) vertex { vb.position = position.Get(); }
  void fragmentShader(FragmentBuiltins fb) fragment {
    fragColor.Set(bindings.Get().uniforms.MapReadUniform().color);
  }
  vertex Buffer<Vertex[]>* position;
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
  BindGroup<Bindings>* bindings;
}
auto uniformBuffer = new uniform Buffer<Uniforms>(device);
Bindings bindings;
bindings.uniforms = uniformBuffer;
auto bg = new BindGroup<Bindings>(device, &bindings);
auto stagingBuffer = new writeonly Buffer<Uniforms>(device);
auto pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
for (int i = 0; i < 1000; ++i) {
  while (System.HasPendingEvents()) {
    System.GetNextEvent();
  }
  auto framebuffer = swapChain.GetCurrentTexture();
  auto encoder = new CommandEncoder(device);
  writeonly Uniforms^ s = stagingBuffer.MapWrite();
  float f = (float) i / 1000.0;
  s.color = float<4>(1.0 - f, f, 0.0, 1.0);
  stagingBuffer.Unmap();
  encoder.CopyBufferToBuffer(stagingBuffer, uniformBuffer);
  Pipeline p;
  p.position = vb;
  p.fragColor = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
  p.bindings = bg;
  auto renderPass = new RenderPass<Pipeline>(encoder, &p);
  renderPass.SetPipeline(pipeline);
  renderPass.Draw(3, 1, 0, 0);
  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();
}
return 0.0;
