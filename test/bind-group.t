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
class ObjectData {
  uniform Buffer<Uniforms>* uniforms;
}

class Pipeline {
  void vertexShader(VertexBuiltins vb, Vertex v) vertex { vb.position = v; }
  void fragmentShader(FragmentBuiltins fb) fragment {
    auto u = objectData.uniforms.MapReadUniform();
    fragColor.Set(u.color);
  }
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
  ObjectData objectData;
}
ObjectData* objectData = new ObjectData();
objectData.uniforms = new uniform Buffer<Uniforms>(device);
BindGroup* bg = new BindGroup(device, objectData);
auto stagingBuffer = new writeonly Buffer<Uniforms>(device);
RenderPipeline* pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
while (System.IsRunning()) {
  Event* event = System.GetNextEvent();
  if (event.type == MouseMove) {
    writeonly Uniforms^ s = stagingBuffer.MapWrite();
    s.color = float<4>((float) event.position.x / 640.0, (float) event.position.y / 480.0, 0.0, 1.0);
    stagingBuffer.Unmap();
  }
  auto encoder = new CommandEncoder(device);
  encoder.CopyBufferToBuffer(stagingBuffer, objectData.uniforms);
  Pipeline p;
  p.fragColor = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
  auto renderPass = new RenderPass<Pipeline>(encoder, &p);
  renderPass.SetPipeline(pipeline);
  renderPass.SetVertexBuffer(0, vb);
  renderPass.SetBindGroup(0, bg);
  renderPass.Draw(3, 1, 0, 0);
  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();
}
return 0.0;
