class Vertex {
  var position : float<2>;
  var color : float<3>;
}

class Uniforms {
  var mvpMatrix : float<4,4>;
  var alpha : float;
}

var device = new Device();
var window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);

var verts = [3] new Vertex;
verts[0].position = float<2>( 0.0,  1.0);
verts[1].position = float<2>(-1.0, -1.0);
verts[2].position = float<2>( 1.0, -1.0);
verts[0].color = float<3>(1.0, 0.0, 0.0);
verts[1].color = float<3>(0.0, 1.0, 0.0);
verts[2].color = float<3>(0.0, 0.0, 1.0);

var vb = new vertex Buffer<[]Vertex>(device, verts);

class Bindings {
  var uniformBuffer : *uniform Buffer<Uniforms>;
}

class Pipeline {
  vertex main(vb : ^VertexBuiltins) : float<4> {
    var uniforms = bindings.Get().uniformBuffer.Map();
    var v = vertices.Get();
    var position = float<4>(v.position.x, v.position.y, 0.0, 1.0);
    vb.position = uniforms.mvpMatrix * position;
    return float<4>(v.color.r, v.color.g, v.color.b, 1.0);
  }
  fragment main(fb : ^FragmentBuiltins, varyings : float<4>) {
    fragColor.Set(varyings * bindings.Get().uniformBuffer.Map().alpha);
  }
  var vertices : *vertex Buffer<[]Vertex>;
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
  var bindings : *BindGroup<Bindings>;
}

var uniformBuffer = new uniform Buffer<Uniforms>(device);
var uniformData : Uniforms;
uniformData.mvpMatrix = float<4, 4>(float<4>(1.0, 0.0, 0.0, 0.0),
                                    float<4>(0.0, 1.0, 0.0, 0.0),
                                    float<4>(0.0, 0.0, 1.0, 0.0),
                                    float<4>(0.0, 0.0, 0.0, 1.0));
uniformData.alpha = 0.5;
var bg = new BindGroup<Bindings>(device, { uniformBuffer } );
var pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
var theta = 0.0;
while (System.IsRunning()) {
  uniformBuffer.SetData(&uniformData);
  while (System.HasPendingEvents()) {
    System.GetNextEvent();
  }
  var encoder = new CommandEncoder(device);
  var fb = swapChain.GetCurrentTexture().CreateColorAttachment(Clear, Store);
  var renderPass = new RenderPass<Pipeline>(encoder, {vertices = vb, fragColor = fb, bindings = bg});
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
}
