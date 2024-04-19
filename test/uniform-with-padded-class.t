using Vertex = float<4>;
Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);
auto verts = new Vertex[3];
verts[0] = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1] = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2] = float<4>( 1.0, -1.0, 0.0, 1.0);
auto vb = new vertex Buffer<Vertex[]>(device, verts);
class Padding {
  float pad1;
}

class Uniforms {
  Padding padding;
  int intPadding;
  float<4> color;
}
class Pipeline {
  void vertexShader(VertexBuiltins vb, Vertex v) vertex { vb.position = v; }
  float<4> fragmentShader(FragmentBuiltins fb) fragment {
    return uniforms.MapReadUniform().color;
  }
  uniform Buffer<Uniforms>* uniforms;
}
auto uniforms = new Uniforms();
uniforms.color = float<4>(0.0, 1.0, 0.0, 1.0);
auto uniformBuffer = new uniform Buffer<Uniforms>(device, uniforms);
BindGroup* bg = new BindGroup(device, uniformBuffer);
auto stagingBuffer = new writeonly Buffer<Uniforms>(device);
RenderPipeline* pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
auto framebuffer = swapChain.GetCurrentTexture();
CommandEncoder* encoder = new CommandEncoder(device);
RenderPassEncoder* passEncoder = encoder.BeginRenderPass(framebuffer);
passEncoder.SetPipeline(pipeline);
passEncoder.SetVertexBuffer(0, vb);
passEncoder.SetBindGroup(0, bg);
passEncoder.Draw(3, 1, 0, 0);
passEncoder.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
return 0.0;
