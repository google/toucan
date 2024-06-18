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
  void vertexShader(VertexBuiltins vb) vertex { vb.position = vert.Get(); }
  void fragmentShader(FragmentBuiltins fb) fragment {
    red.Set(float<4>(1.0, 0.0, 0.0, 1.0));
    green.Set(float<4>(0.0, 1.0, 0.0, 1.0));
  }
  ColorAttachment<RGBA8unorm>* red;
  ColorAttachment<RGBA8unorm>* green;
  vertex Buffer<Vertex[]>* vert;
}

class Bindings {
  Sampler*                    sampler;
  SampleableTexture2D<float>* red;
  SampleableTexture2D<float>* green;
}

class Pipeline {
  void vertexShader(VertexBuiltins vb) vertex { vb.position = vert.Get(); }
  void fragmentShader(FragmentBuiltins fb) fragment {
    auto b = bindings.Get();
    auto onehalf = float<2>(0.5, 0.5);
    fragColor.Set(b.red.Sample(b.sampler, onehalf) + b.green.Sample(b.sampler, onehalf));
  }
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
  vertex Buffer<Vertex[]>* vert;
  BindGroup<Bindings>* bindings;
}
auto encoder = new CommandEncoder(device);
auto redTex = new sampleable renderable Texture2D<RGBA8unorm>(device, 640, 480);
auto greenTex = new sampleable renderable Texture2D<RGBA8unorm>(device, 640, 480);
RTTPipeline rp;
rp.red = new ColorAttachment<RGBA8unorm>(redTex, Clear, Store);
rp.green = new ColorAttachment<RGBA8unorm>(greenTex, Clear, Store);
rp.vert = vb;
auto rttPass = new RenderPass<RTTPipeline>(encoder, &rp);
auto rttPipeline = new RenderPipeline<RTTPipeline>(device, null, TriangleList);
rttPass.SetPipeline(rttPipeline);
rttPass.Draw(3, 1, 0, 0);
rttPass.End();
auto pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
Pipeline p;
p.vert = vb;
p.fragColor = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
Bindings b;
b.sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);
b.red = redTex.CreateSampleableView();
b.green = greenTex.CreateSampleableView();
p.bindings = new BindGroup<Bindings>(device, &b);
auto drawPass = new RenderPass<Pipeline>(encoder, &p);
drawPass.SetPipeline(pipeline);
drawPass.Draw(3, 1, 0, 0);
drawPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
