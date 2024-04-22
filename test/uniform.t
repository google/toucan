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
class PipelineData {
  void vertexShader(VertexBuiltins vb, float<4> v) vertex { vb.position = v; }
  float<4> fragmentShader(FragmentBuiltins fb) fragment {
    return float<4>(0.0, 1.0, 0.0, 1.0) * uniforms.MapReadUniform().opacity;
  }
  uniform Buffer<UniformData>* uniforms;
}
auto pipeline = new RenderPipeline<PipelineData>(device, null, TriangleList);
UniformData* uniforms = new UniformData();
uniforms.opacity = 0.5;
auto uniformBuffer = new uniform Buffer<UniformData>(device, uniforms);
auto bindGroup = new BindGroup(device, uniformBuffer);
auto framebuffer = swapChain.GetCurrentTexture();
auto encoder = new CommandEncoder(device);
auto renderPass = new RenderPass(encoder, framebuffer);
renderPass.SetPipeline(pipeline);
renderPass.SetBindGroup(0, bindGroup);
renderPass.SetVertexBuffer(0, vb);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
return 0.0;
