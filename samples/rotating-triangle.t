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

auto vb = new vertex Buffer<Vertex[]>(device, verts.length);
vb.SetData(verts);

class Pipeline {
  float<4> vertexShader(VertexBuiltins vb, Vertex vtx) vertex {
    auto uniforms = uniformBuffer.MapReadUniform();
    auto position = float<4>(vtx.position.x, vtx.position.y, 0.0, 1.0);
    vb.position = uniforms.mvpMatrix * position;
    return float<4>(vtx.color.r, vtx.color.g, vtx.color.b, 1.0);
  }
  float<4> fragmentShader(FragmentBuiltins fb, float<4> varyings) fragment {
    return varyings * uniformBuffer.MapReadUniform().alpha;
  }

  uniform Buffer<Uniforms>* uniformBuffer;
}

auto uniformBuffer = new uniform Buffer<Uniforms>(device);
Uniforms uniformData;
uniformData.mvpMatrix = float<4, 4>(float<4>(1.0, 0.0, 0.0, 0.0),
                                    float<4>(0.0, 1.0, 0.0, 0.0),
                                    float<4>(0.0, 0.0, 1.0, 0.0),
                                    float<4>(0.0, 0.0, 0.0, 1.0));
uniformData.alpha = 0.5;
auto bg = new BindGroup(device, uniformBuffer);
RenderPipeline* pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
float theta = 0.0;
while (System.IsRunning()) {
  uniformBuffer.SetData(&uniformData);
  while (System.HasPendingEvents()) {
    System.GetNextEvent();
  }
  auto framebuffer = swapChain.GetCurrentTexture();
  CommandEncoder* encoder = new CommandEncoder(device);
  RenderPassEncoder* passEncoder = encoder.BeginRenderPass(framebuffer);
  passEncoder.SetPipeline(pipeline);
  passEncoder.SetVertexBuffer(0, vb);
  passEncoder.SetBindGroup(0, bg);
  passEncoder.Draw(3, 1, 0, 0);
  passEncoder.End();
  CommandBuffer* cb = encoder.Finish();
  device.GetQueue().Submit(cb);
  swapChain.Present();

  theta += 0.001;
  uniformData.mvpMatrix[0][0] = Math.cos(theta);
  uniformData.mvpMatrix[1][1] = uniformData.mvpMatrix[0][0];
  uniformData.mvpMatrix[1][0] = Math.sin(theta);
  uniformData.mvpMatrix[0][1] = -uniformData.mvpMatrix[1][0];
}
return 0.0;
