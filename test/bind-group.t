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
  float<4> fragmentShader(FragmentBuiltins fb) fragment {
    auto u = objectData.uniforms.MapReadUniform();
    return u.color;
  }
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
  auto framebuffer = swapChain.GetCurrentTexture();
  CommandEncoder* encoder = new CommandEncoder(device);
  encoder.CopyBufferToBuffer(stagingBuffer, objectData.uniforms);
  RenderPassEncoder* passEncoder = encoder.BeginRenderPass(framebuffer);
  passEncoder.SetPipeline(pipeline);
  passEncoder.SetVertexBuffer(0, vb);
  passEncoder.SetBindGroup(0, bg);
  passEncoder.Draw(3, 1, 0, 0);
  passEncoder.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();
}
return 0.0;
