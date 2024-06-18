using Vertex = float<4>;
Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);
Vertex[3] verts = { { 0.0, 1.0, 0.0, 1.0 }, {-1.0, -1.0, 0.0, 1.0 }, { 1.0, -1.0, 0.0, 1.0 } };
auto vb = new vertex Buffer<Vertex[]>(device, &verts);
class Padding {
  float pad1;
}

class Uniforms {
  Padding padding;
  int intPadding;
  float<4> color;
}
class Bindings {
  uniform Buffer<Uniforms>* uniforms;
}
class Pipeline {
  void vertexShader(VertexBuiltins vb) vertex { vb.position = vertices.Get(); }
  void fragmentShader(FragmentBuiltins fb) fragment {
    fragColor.Set(bindings.Get().uniforms.MapReadUniform().color);
  }
  vertex Buffer<Vertex[]>* vertices;
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
  BindGroup<Bindings>* bindings;
}
auto uniforms = new uniform Buffer<Uniforms>(device, { color = { 0.0, 1.0, 0.0, 1.0 } });
auto bg = new BindGroup<Bindings>(device, { uniforms });
auto stagingBuffer = new writeonly Buffer<Uniforms>(device);
auto pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
auto framebuffer = swapChain.GetCurrentTexture();
auto encoder = new CommandEncoder(device);
auto fb = new ColorAttachment<PreferredSwapChainFormat>(framebuffer, Clear, Store);
auto renderPass = new RenderPass<Pipeline>(encoder,
  {vertices = vb, fragColor = fb, bindings = bg }
);
renderPass.SetPipeline(pipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
return 0.0;
