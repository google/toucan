using Vertex = float<2>;

class RTTPipeline {
  vertex main(vb : &VertexBuiltins) { vb.position = {@vert.Get(), 0.0, 1.0}; }
  fragment main(fb : &FragmentBuiltins) {
    red.Set(float<4>(1.0, 0.0, 0.0, 1.0));
    green.Set(float<4>(0.0, 1.0, 0.0, 1.0));
  }
  var red : *ColorAttachment<RGBA8unorm>;
  var green : *ColorAttachment<RGBA8unorm>;
  var vert : *VertexInput<Vertex>;
}

class Bindings {
  var sampler : *Sampler;
  var red : *SampleableTexture2D<float>;
  var green : *SampleableTexture2D<float>;
}

class Pipeline {
  vertex main(vb : &VertexBuiltins) { vb.position = {@vert.Get(), 0.0, 1.0}; }
  fragment main(fb : &FragmentBuiltins) {
    var b = bindings.Get();
    var onehalf = float<2>(0.5, 0.5);
    fragColor.Set(b.red.Sample(b.sampler, onehalf) + b.green.Sample(b.sampler, onehalf));
  }
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
  var vert : *VertexInput<Vertex>;
  var bindings : *BindGroup<Bindings>;
}

var device = new Device();
var window = new Window({640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = [3]Vertex{ {0.0, 1.0}, {-1.0, -1.0}, {1.0, -1.0} };
var vb = new vertex Buffer<[]Vertex>(device, &verts);
var encoder = new CommandEncoder(device);
var redTex = new sampleable renderable Texture2D<RGBA8unorm>(device, window.GetSize());
var greenTex = new sampleable renderable Texture2D<RGBA8unorm>(device, window.GetSize());
var rp : RTTPipeline;
rp.red = redTex.CreateColorAttachment(LoadOp.Clear);
rp.green = greenTex.CreateColorAttachment(LoadOp.Clear);
rp.vert = new VertexInput<Vertex>(vb);
var rttPass = new RenderPass<RTTPipeline>(encoder, &rp);
var rttPipeline = new RenderPipeline<RTTPipeline>(device);
rttPass.SetPipeline(rttPipeline);
rttPass.Draw(3, 1, 0, 0);
rttPass.End();
var pipeline = new RenderPipeline<Pipeline>(device);
var b = new BindGroup<Bindings>(device, {
  sampler = new Sampler(device),
  red = redTex.CreateSampleableView(),
  green = greenTex.CreateSampleableView()
});
var p = Pipeline{
  vert = new VertexInput<Vertex>(vb),
  fragColor = swapChain.GetCurrentTexture().CreateColorAttachment(LoadOp.Clear),
  bindings = b
};
var drawPass = new RenderPass<Pipeline>(encoder, &p);
drawPass.SetPipeline(pipeline);
drawPass.Draw(3, 1, 0, 0);
drawPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
