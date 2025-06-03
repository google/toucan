using Vertex = float<2>;

class Uniforms {
  var color : float<4>;
}

class ObjectData {
  var uniforms : *uniform Buffer<Uniforms>;
}

class Pipeline {
  vertex main(vb : &VertexBuiltins) { vb.position = {@vertices.Get(), 0.0, 1.0}; }
  fragment main(fb : &FragmentBuiltins) {
    fragColor.Set(objectData.Get().uniforms.MapRead().color);
  }
  var vertices : *VertexInput<Vertex>;
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
  var objectData : *BindGroup<ObjectData>;
}
var device = new Device();
var window = new Window({640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = [3]Vertex{ { 0.0,  1.0 }, {-1.0, -1.0 }, { 1.0, -1.0 } };
var vb = new vertex Buffer<[]Vertex>(device, &verts);
var objectData = ObjectData{ new uniform Buffer<Uniforms>(device) };
var bg = new BindGroup<ObjectData>(device, &objectData);
var stagingBuffer = new hostwriteable Buffer<Uniforms>(device);
var pipeline = new RenderPipeline<Pipeline>(device);
while (System.IsRunning()) {
  var encoder = new CommandEncoder(device);
  objectData.uniforms.CopyFromBuffer(encoder, stagingBuffer);
  var p = Pipeline{
    vertices = new VertexInput<Vertex>(vb),
    fragColor = swapChain.GetCurrentTexture().CreateColorAttachment(LoadOp.Clear),
    objectData = bg
  };
  var renderPass = new RenderPass<Pipeline>(encoder, &p);
  renderPass.SetPipeline(pipeline);
  renderPass.Draw(3, 1, 0, 0);
  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();
  while (System.HasPendingEvents()) {
    var event = System.GetNextEvent();
    if (event.type == EventType.MouseMove) {
      var s = stagingBuffer.MapWrite();
      s.color = float<4>((float) event.position.x / 640.0, (float) event.position.y / 480.0, 0.0, 1.0);
    }
  }
}
