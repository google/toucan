using Vertex = float<4>;
var device = new Device();
var window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = new Vertex[3];
verts[0] = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1] = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2] = float<4>( 1.0, -1.0, 0.0, 1.0);
var vb = new vertex Buffer<Vertex[]>(device, verts);
class Uniforms {
  var color : float<4>;
}
class ObjectData {
  var uniforms : *uniform Buffer<Uniforms>;
}

class Pipeline {
  vertex main(vb : ^VertexBuiltins) { vb.position = vert.Get(); }
  fragment main(fb : ^FragmentBuiltins) {
    var u = objectData.Get().uniforms.Map();
    fragColor.Set(u.color);
  }
  var vert : *vertex Buffer<Vertex[]>;
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
  var objectData : *BindGroup<ObjectData>;
}
var objectData : ObjectData;
objectData.uniforms = new uniform Buffer<Uniforms>(device);
var bg = new BindGroup<ObjectData>(device, &objectData);
var stagingBuffer = new writeonly Buffer<Uniforms>(device);
var pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
while (System.IsRunning()) {
  var event = System.GetNextEvent();
  if (event.type == MouseMove) {
    var s = stagingBuffer.Map();
    s.color = float<4>((float) event.position.x / 640.0, (float) event.position.y / 480.0, 0.0, 1.0);
    stagingBuffer.Unmap();
  }
  var encoder = new CommandEncoder(device);
  objectData.uniforms.CopyFromBuffer(encoder, stagingBuffer);
  var p : Pipeline;
  p.vert = vb;
  p.fragColor = swapChain.GetCurrentTexture().CreateColorAttachment(Clear, Store);
  p.objectData = bg;
  var renderPass = new RenderPass<Pipeline>(encoder, &p);
  renderPass.SetPipeline(pipeline);
  renderPass.Draw(3, 1, 0, 0);
  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();
}
