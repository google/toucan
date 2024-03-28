Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
Queue* queue = device.GetQueue();
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);
float<4>[]* verts = new float<4>[3];
verts[0] = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1] = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2] = float<4>( 1.0, -1.0, 0.0, 1.0);
auto vb = new vertex Buffer<float<4>[]>(device, verts);
class UniformData {
  float opacity;
}
class Bindings {
  uniform Buffer<UniformData>* uniforms;
}
class PipelineData {
  void vertexShader(VertexBuiltins vb) vertex { vb.position = vertices.Get(); }
  void fragmentShader(FragmentBuiltins fb) fragment {
    fragColor.Set(float<4>(0.0, 1.0, 0.0, 1.0) * bindings.Get().uniforms.MapReadUniform().opacity);
  }
  vertex Buffer<float<4>[]>* vertices;
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
  BindGroup<Bindings>* bindings;
}
auto pipeline = new RenderPipeline<PipelineData>(device, null, TriangleList);
auto uniformBuffer = new uniform Buffer<UniformData>(device, { opacity = 0.5 });
auto bindGroup = new BindGroup<Bindings>(device, { uniformBuffer });
auto framebuffer = swapChain.GetCurrentTexture();
auto encoder = new CommandEncoder(device);
auto fb = new ColorAttachment<PreferredSwapChainFormat>(framebuffer, Clear, Store);
auto renderPass = new RenderPass<PipelineData>(encoder, { vertices = vb, fragColor = fb, bindings = bindGroup});
renderPass.SetPipeline(pipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
return 0.0;
