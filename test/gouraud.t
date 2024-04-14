class Vertex {
  float<2> position;
  float<3> color;
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
  float<3> vertexShader(VertexBuiltins vb, Vertex vtx) vertex {
    vb.position = float<4>(vtx.position.x, vtx.position.y, 0.0, 1.0);
    return vtx.color;
  }
  float<4> fragmentShader(FragmentBuiltins fb, float<3> varyings) fragment {
    return float<4>(varyings.r, varyings.g, varyings.b, 1.0);
  }
}

RenderPipeline* pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
auto framebuffer = swapChain.GetCurrentTexture();
CommandEncoder* encoder = new CommandEncoder(device);
RenderPassEncoder* passEncoder = encoder.BeginRenderPass(framebuffer);
passEncoder.SetPipeline(pipeline);
passEncoder.SetVertexBuffer(0, vb);
passEncoder.Draw(3, 1, 0, 0);
passEncoder.End();
CommandBuffer* cb = encoder.Finish();
device.GetQueue().Submit(cb);
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
return 0.0;
