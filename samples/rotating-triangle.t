class Vertex {
  float<2> position;
  float<3> color;
}

class Uniforms {
  float<4,4> mvpMatrix;
  float alpha;
}

Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);

auto verts = new Vertex[3];
verts[0].position = float<2>( 0.0,  1.0);
verts[1].position = float<2>(-1.0, -1.0);
verts[2].position = float<2>( 1.0, -1.0);
verts[0].color = float<3>(1.0, 0.0, 0.0);
verts[1].color = float<3>(0.0, 1.0, 0.0);
verts[2].color = float<3>(0.0, 0.0, 1.0);

auto vb = new vertex Buffer<Vertex[]>(device, verts);

class Bindings {
  uniform Buffer<Uniforms>* uniformBuffer;
}

class Pipeline {
  float<4> vertexShader(VertexBuiltins vb) vertex {
    auto uniforms = bindings.Get().uniformBuffer.MapReadUniform();
    auto v = vertices.Get();
    auto position = float<4>(v.position.x, v.position.y, 0.0, 1.0);
    vb.position = uniforms.mvpMatrix * position;
    return float<4>(v.color.r, v.color.g, v.color.b, 1.0);
  }
  void fragmentShader(FragmentBuiltins fb, float<4> varyings) fragment {
    fragColor.Set(varyings * bindings.Get().uniformBuffer.MapReadUniform().alpha);
  }
  vertex Buffer<Vertex[]>* vertices;
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
  BindGroup<Bindings>* bindings;
}

auto uniformBuffer = new uniform Buffer<Uniforms>(device);
Uniforms uniformData;
uniformData.mvpMatrix = float<4, 4>(float<4>(1.0, 0.0, 0.0, 0.0),
                                    float<4>(0.0, 1.0, 0.0, 0.0),
                                    float<4>(0.0, 0.0, 1.0, 0.0),
                                    float<4>(0.0, 0.0, 0.0, 1.0));
uniformData.alpha = 0.5;
Bindings bindings;
bindings.uniformBuffer = uniformBuffer;
auto bg = new BindGroup<Bindings>(device, &bindings);
auto pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
float theta = 0.0;
while (System.IsRunning()) {
  uniformBuffer.SetData(&uniformData);
  while (System.HasPendingEvents()) {
    System.GetNextEvent();
  }
  auto framebuffer = swapChain.GetCurrentTexture();
  auto encoder = new CommandEncoder(device);
  auto fb = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
  auto renderPass = new RenderPass<Pipeline>(encoder, {vertices = vb, fragColor = fb, bindings = bg});
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
return 0.0;
