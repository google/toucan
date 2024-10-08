using Vertex = float<4>;
Device* device = new Device();
Window* window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = new Vertex[3];
verts[0] = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1] = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2] = float<4>( 1.0, -1.0, 0.0, 1.0);
var vb = new vertex Buffer<Vertex[]>(device, verts);
class Uniforms {
  float<4> color;
}
class ObjectData {
  uniform Buffer<Uniforms>* uniforms;
}

class Pipeline {
  void vertexShader(VertexBuiltins^ vb) vertex { vb.position = vert.Get(); }
  void fragmentShader(FragmentBuiltins^ fb) fragment {
    var u = objectData.Get().uniforms.MapReadUniform();
    fragColor.Set(u.color);
  }
  vertex Buffer<Vertex[]>* vert;
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
  BindGroup<ObjectData>* objectData;
}
ObjectData objectData;
objectData.uniforms = new uniform Buffer<Uniforms>(device);
var bg = new BindGroup<ObjectData>(device, &objectData);
var stagingBuffer = new writeonly Buffer<Uniforms>(device);
var pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
while (System.IsRunning()) {
  Event* event = System.GetNextEvent();
  if (event.type == MouseMove) {
    writeonly Uniforms^ s = stagingBuffer.MapWrite();
    s.color = float<4>((float) event.position.x / 640.0, (float) event.position.y / 480.0, 0.0, 1.0);
    stagingBuffer.Unmap();
  }
  var encoder = new CommandEncoder(device);
  encoder.CopyBufferToBuffer(stagingBuffer, objectData.uniforms);
  Pipeline p;
  p.vert = vb;
  p.fragColor = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
  p.objectData = bg;
  var renderPass = new RenderPass<Pipeline>(encoder, &p);
  renderPass.SetPipeline(pipeline);
  renderPass.Draw(3, 1, 0, 0);
  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();
}
