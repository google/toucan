using Vertex = float<4>;
Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);
auto verts = new Vertex[3];
verts[0] = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1] = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2] = float<4>( 1.0, -1.0, 0.0, 1.0);
auto vb = new vertex Buffer<Vertex[]>(device, verts);
class RTTPipeline {
  void vertexShader(VertexBuiltins vb, Vertex v) vertex { vb.position = v; }
  void fragmentShader(FragmentBuiltins fb) fragment {
    red.Set(float<4>(1.0, 0.0, 0.0, 1.0));
    green.Set(float<4>(0.0, 1.0, 0.0, 1.0));
  }
  ColorAttachment<RGBA8unorm>* red;
  ColorAttachment<RGBA8unorm>* green;
}
class Pipeline {
  void vertexShader(VertexBuiltins vb, Vertex v) vertex { vb.position = v; }
  void fragmentShader(FragmentBuiltins fb) fragment {
    auto onehalf = float<2>(0.5, 0.5);
    fragColor.Set(red.Sample(sampler, onehalf) + green.Sample(sampler, onehalf));
  }
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
  Sampler*                    sampler;
  SampleableTexture2D<float>* red;
  SampleableTexture2D<float>* green;
}
auto encoder = new CommandEncoder(device);
auto redTex = new sampleable renderable Texture2D<RGBA8unorm>(device, 640, 480);
auto greenTex = new sampleable renderable Texture2D<RGBA8unorm>(device, 640, 480);
RTTPipeline rp;
rp.red = new ColorAttachment<RGBA8unorm>(redTex, Clear, Store);
rp.green = new ColorAttachment<RGBA8unorm>(greenTex, Clear, Store);
auto rttPass = new RenderPass<RTTPipeline>(encoder, &rp);
RenderPipeline* rttPipeline = new RenderPipeline<RTTPipeline>(device, null, TriangleList);
rttPass.SetPipeline(rttPipeline);
rttPass.SetVertexBuffer(0, vb);
rttPass.Draw(3, 1, 0, 0);
rttPass.End();
RenderPipeline* pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
Pipeline p;
p.fragColor = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
auto drawPass = new RenderPass<Pipeline>(encoder, &p);
drawPass.SetPipeline(pipeline);
drawPass.SetVertexBuffer(0, vb);
auto sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);
drawPass.SetBindGroup(0, new BindGroup(device, sampler));
drawPass.SetBindGroup(1, new BindGroup(device, redTex.CreateSampleableView()));
drawPass.SetBindGroup(2, new BindGroup(device, greenTex.CreateSampleableView()));
drawPass.Draw(3, 1, 0, 0);
drawPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
return 0.0;
