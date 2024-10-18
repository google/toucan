using Vertex = float<4>;
var device = new Device();
var window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = new Vertex[3];
verts[0] = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1] = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2] = float<4>( 1.0, -1.0, 0.0, 1.0);
var vb = new vertex Buffer<Vertex[]>(device, verts);
class RTTPipeline {
  vertex main(vb : VertexBuiltins^) { vb.position = vert.Get(); }
  fragment main(fb : FragmentBuiltins^) {
    red.Set(float<4>(1.0, 0.0, 0.0, 1.0));
    green.Set(float<4>(0.0, 1.0, 0.0, 1.0));
  }
  var red : ColorAttachment<RGBA8unorm>*;
  var green : ColorAttachment<RGBA8unorm>*;
  var vert : vertex Buffer<Vertex[]>*;
}

class Bindings {
  var sampler : Sampler*;
  var red : SampleableTexture2D<float>*;
  var green : SampleableTexture2D<float>*;
}

class Pipeline {
  vertex main(vb : VertexBuiltins^) { vb.position = vert.Get(); }
  fragment main(fb : FragmentBuiltins^) {
    var b = bindings.Get();
    var onehalf = float<2>(0.5, 0.5);
    fragColor.Set(b.red.Sample(b.sampler, onehalf) + b.green.Sample(b.sampler, onehalf));
  }
  var fragColor : ColorAttachment<PreferredSwapChainFormat>*;
  var vert : vertex Buffer<Vertex[]>*;
  var bindings : BindGroup<Bindings>*;
}
var encoder = new CommandEncoder(device);
var redTex = new sampleable renderable Texture2D<RGBA8unorm>(device, window.GetSize());
var greenTex = new sampleable renderable Texture2D<RGBA8unorm>(device, window.GetSize());
var rp : RTTPipeline;
rp.red = redTex.CreateColorAttachment(Clear, Store);
rp.green = greenTex.CreateColorAttachment(Clear, Store);
rp.vert = vb;
var rttPass = new RenderPass<RTTPipeline>(encoder, &rp);
var rttPipeline = new RenderPipeline<RTTPipeline>(device, null, TriangleList);
rttPass.SetPipeline(rttPipeline);
rttPass.Draw(3, 1, 0, 0);
rttPass.End();
var pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
var p : Pipeline;
p.vert = vb;
p.fragColor = swapChain.GetCurrentTexture().CreateColorAttachment(Clear, Store);
var b : Bindings;
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
