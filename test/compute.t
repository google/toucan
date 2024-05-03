using Vertex = float<4>;

class ComputeBindings {
  storage Buffer<Vertex[]>* vertStorage;
}

class BumpCompute {
  void computeShader(ComputeBuiltins cb) compute(1, 1, 1) {
    auto verts = bindings.Get().vertStorage.MapReadWriteStorage();
    uint pos = cb.globalInvocationId.x;
    verts[pos] += float<4>( 0.005,  0.0, 0.0, 0.0);
  }
  BindGroup<ComputeBindings>* bindings;
}

Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);
auto verts = new Vertex[3];
verts[0] = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1] = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2] = float<4>( 1.0, -1.0, 0.0, 1.0);
auto vb = new vertex storage Buffer<Vertex[]>(device, verts);
class Pipeline {
  void vertexShader(VertexBuiltins vb) vertex { vb.position = vert.Get(); }
  void fragmentShader(FragmentBuiltins fb) fragment { fragColor.Set(float<4>(0.0, 1.0, 0.0, 1.0)); }
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
  vertex Buffer<Vertex[]>* vert;
}

auto pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
ComputePipeline* computePipeline = new ComputePipeline<BumpCompute>(device);

ComputeBindings cb;
cb.vertStorage = vb;
auto storageBG = new BindGroup<ComputeBindings>(device, &cb);

while (System.IsRunning()) {
  auto encoder = new CommandEncoder(device);
  auto computePass = new ComputePass<BumpCompute>(encoder, null);
  computePass.SetPipeline(computePipeline);
  BumpCompute bc;
  bc.bindings = storageBG;
  computePass.Set(&bc);
  computePass.Dispatch(verts.length, 1, 1);
  computePass.End();
  Pipeline p;
  p.vert = vb;
  p.fragColor = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
  auto renderPass = new RenderPass<Pipeline>(encoder, &p);
  renderPass.SetPipeline(pipeline);
  renderPass.Draw(3, 1, 0, 0);
  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();

  while (System.HasPendingEvents()) System.GetNextEvent();
}
return 0.0;
