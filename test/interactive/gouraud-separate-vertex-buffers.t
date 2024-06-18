Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);

float<2>[3] positions;
positions[0] = float<2>( 0.0,  1.0);
positions[1] = float<2>(-1.0, -1.0);
positions[2] = float<2>( 1.0, -1.0);

float<3>[3] colors;
colors[0] = float<3>(1.0, 0.0, 0.0);
colors[1] = float<3>(0.0, 1.0, 0.0);
colors[2] = float<3>(0.0, 0.0, 1.0);

class Pipeline {
  float<3> vertexShader(VertexBuiltins vb) vertex {
    float<2> v = position.Get();
    vb.position = float<4>(v.x, v.y, 0.0, 1.0);
    return color.Get();
  }
  void fragmentShader(FragmentBuiltins fb, float<3> varyings) fragment {
    fragColor.Set(float<4>(varyings.r, varyings.g, varyings.b, 1.0));
  }
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
  vertex Buffer<float<2>[]>* position;
  vertex Buffer<float<3>[]>* color;
}

auto pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
auto encoder = new CommandEncoder(device);
Pipeline p;
p.fragColor = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
p.position = new vertex Buffer<float<2>[]>(device, &positions);
p.color = new vertex Buffer<float<3>[]>(device, &colors);
auto renderPass = new RenderPass<Pipeline>(encoder, &p);
renderPass.SetPipeline(pipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
return 0.0;
