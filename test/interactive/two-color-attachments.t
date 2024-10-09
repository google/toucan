using Vertex = float<4>;
Device* device = new Device();
Window* window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = new Vertex[3];
verts[0] = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1] = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2] = float<4>( 1.0, -1.0, 0.0, 1.0);
var vb = new vertex Buffer<Vertex[]>(device, verts);
class RTTPipeline {
  void vertexShader(VertexBuiltins^ vb) vertex { vb.position = vert.Get(); }
  void fragmentShader(FragmentBuiltins^ fb) fragment {
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
  void vertexShader(VertexBuiltins^ vb) vertex { vb.position = vert.Get(); }
  void fragmentShader(FragmentBuiltins^ fb) fragment {
    var b = bindings.Get();
    var onehalf = float<2>(0.5, 0.5);
    fragColor.Set(b.red.Sample(b.sampler, onehalf) + b.green.Sample(b.sampler, onehalf));
  }
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
  vertex Buffer<Vertex[]>* vert;
  BindGroup<Bindings>* bindings;
}
var encoder = new CommandEncoder(device);
var redTex = new sampleable renderable Texture2D<RGBA8unorm>(device, window.GetSize());
var greenTex = new sampleable renderable Texture2D<RGBA8unorm>(device, window.GetSize());
RTTPipeline rp;
rp.red = new ColorAttachment<RGBA8unorm>(redTex, Clear, Store);
rp.green = new ColorAttachment<RGBA8unorm>(greenTex, Clear, Store);
rp.vert = vb;
var rttPass = new RenderPass<RTTPipeline>(encoder, &rp);
var rttPipeline = new RenderPipeline<RTTPipeline>(device, null, TriangleList);
rttPass.SetPipeline(rttPipeline);
rttPass.Draw(3, 1, 0, 0);
rttPass.End();
var pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
Pipeline p;
p.vert = vb;
p.fragColor = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
Bindings b;
b.sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);
b.red = redTex.CreateSampleableView();
b.green = greenTex.CreateSampleableView();
p.bindings = new BindGroup<Bindings>(device, &b);
var drawPass = new RenderPass<Pipeline>(encoder, &p);
drawPass.SetPipeline(pipeline);
drawPass.Draw(3, 1, 0, 0);
drawPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
