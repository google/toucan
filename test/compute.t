using Vertex = float<4>;

class BumpCompute {
  void computeShader(ComputeBuiltins cb) compute(1, 1, 1) {
    auto verts = vertStorage.MapReadWriteStorage();
    uint pos = cb.globalInvocationId.x;
    verts[pos] += float<4>( 0.005,  0.0, 0.0, 0.0);
  }

  storage Buffer<Vertex[]>* vertStorage;
}

Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
SwapChain* swapChain = new SwapChain(window);
auto verts = new Vertex[3];
verts[0] = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1] = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2] = float<4>( 1.0, -1.0, 0.0, 1.0);
auto vb = new vertex storage Buffer<Vertex[]>(device, verts.length);
vb.SetData(verts);
class Pipeline {
  void vertexShader(VertexBuiltins vb, Vertex v) vertex { vb.position = v; }
  float<4> fragmentShader(FragmentBuiltins fb) fragment { return float<4>(0.0, 1.0, 0.0, 1.0); }
}

RenderPipeline* pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
ComputePipeline* computePipeline = new ComputePipeline<BumpCompute>(device);

auto storageBG = new BindGroup(device, vb);

while (System.IsRunning()) {
  while (System.HasPendingEvents()) {
    System.GetNextEvent();
  }
  renderable Texture2DView* framebuffer = swapChain.GetCurrentTextureView();
  CommandEncoder* encoder = new CommandEncoder(device);
  ComputePassEncoder* computeEncoder = encoder.BeginComputePass();
  computeEncoder.SetPipeline(computePipeline);
  computeEncoder.SetBindGroup(0, storageBG);
  computeEncoder.Dispatch(verts.length, 1, 1);
  computeEncoder.End();
  RenderPassEncoder* passEncoder = encoder.BeginRenderPass(framebuffer);
  passEncoder.SetPipeline(pipeline);
  passEncoder.SetVertexBuffer(0, vb);
  passEncoder.Draw(3, 1, 0, 0);
  passEncoder.End();
  CommandBuffer* cb = encoder.Finish();
  device.GetQueue().Submit(cb);
  swapChain.Present();
}
return 0.0;
