class Vertex {
  var position : float<2>;
  var color : float<3>;
}

class Uniforms {
  var mvpMatrix : float<4,4>;
  var alpha : float;
}

class Bindings {
  var uniformBuffer : *uniform Buffer<Uniforms>;
}

class Pipeline {
  vertex main(vb : &VertexBuiltins) : float<4> {
    var uniforms = bindings.Get().uniformBuffer.MapRead();
    var v = vertices.Get();
    vb.position = uniforms.mvpMatrix * float<4>{@v.position, 0.0, 1.0};
    return float<4>{@v.color, 1.0};
  }
  fragment main(fb : &FragmentBuiltins, varyings : float<4>) {
    fragColor.Set(varyings * bindings.Get().uniformBuffer.MapRead().alpha);
  }
  var vertices : *VertexInput<Vertex>;
  var fragColor : *ColorOutput<PreferredPixelFormat>;
  var bindings : *BindGroup<Bindings>;
}

var device = new Device();
var window = new Window({640, 480});
var swapChain = new SwapChain<PreferredPixelFormat>(device, window);

var verts = [3]Vertex{
  { position = { 0.0,  1.0}, color = {1.0, 0.0, 0.0} },
  { position = {-1.0, -1.0}, color = {0.0, 1.0, 0.0} },
  { position = { 1.0, -1.0}, color = {0.0, 0.0, 1.0} }
};

var vb = new vertex Buffer<[]Vertex>(device, &verts);

var uniformBuffer = new uniform Buffer<Uniforms>(device);
var uniformData : Uniforms;
uniformData.mvpMatrix = {{1.0, 0.0, 0.0, 0.0},
                         {0.0, 1.0, 0.0, 0.0},
                         {0.0, 0.0, 1.0, 0.0},
                         {0.0, 0.0, 0.0, 1.0}};
uniformData.alpha = 0.5;
var bg = new BindGroup<Bindings>(device, { uniformBuffer } );
var pipeline = new RenderPipeline<Pipeline>(device);
var theta = 0.0;
while (System.IsRunning()) {
  uniformBuffer.SetData(&uniformData);
  var encoder = new CommandEncoder(device);
  var fb = swapChain.GetCurrentTexture().CreateColorOutput(LoadOp.Clear);
  var vi = new VertexInput<Vertex>(vb);
  var renderPass = new RenderPass<Pipeline>(encoder, {vertices = vi, fragColor = fb, bindings = bg});
  renderPass.SetPipeline(pipeline);
  renderPass.Draw(3, 1, 0, 0);
  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();

  theta += 0.001;
  uniformData.mvpMatrix[0][0] = Math.cos(theta);
  uniformData.mvpMatrix[1][1] = uniformData.mvpMatrix[0][0];
  uniformData.mvpMatrix[1][0] = Math.sin(theta);
  uniformData.mvpMatrix[0][1] = -uniformData.mvpMatrix[1][0];
  while (System.HasPendingEvents()) {
    System.GetNextEvent();
  }
}
