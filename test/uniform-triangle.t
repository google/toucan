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
class Pipeline {
  void vertexShader(VertexBuiltins vb, Vertex v) vertex { vb.position = v; }
  float<4> fragmentShader(FragmentBuiltins fb) fragment {
    auto u = uniforms.MapReadUniform();
    return u.color;
  }
  uniform Buffer<Uniforms>* uniforms;
}
auto uniformBuffer = new uniform Buffer<Uniforms>(device);
BindGroup* bg = new BindGroup(device, uniformBuffer);
auto stagingBuffer = new writeonly Buffer<Uniforms>(device);
RenderPipeline* pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
for (int i = 0; i < 1000; ++i) {
  while (System.HasPendingEvents()) {
    System.GetNextEvent();
  }
  auto framebuffer = swapChain.GetCurrentTexture();
  CommandEncoder* encoder = new CommandEncoder(device);
  writeonly Uniforms^ s = stagingBuffer.MapWrite();
  float f = (float) i / 1000.0;
  s.color = float<4>(1.0 - f, f, 0.0, 1.0);
  stagingBuffer.Unmap();
  encoder.CopyBufferToBuffer(stagingBuffer, uniformBuffer);
  RenderPassEncoder* passEncoder = encoder.BeginRenderPass(framebuffer);
  passEncoder.SetPipeline(pipeline);
  passEncoder.SetVertexBuffer(0, vb);
  passEncoder.SetBindGroup(0, bg);
  passEncoder.Draw(3, 1, 0, 0);
  passEncoder.End();
  CommandBuffer* cb = encoder.Finish();
  device.GetQueue().Submit(cb);
  swapChain.Present();
}
return 0.0;
