Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
Queue* queue = device.GetQueue();
SwapChain* swapChain = new SwapChain(window);
float<4>[] verts = new float<4>[3];
verts[0] = float<4>( 0.0,  1.0, 0.0, 1.0);
verts[1] = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[2] = float<4>( 1.0, -1.0, 0.0, 1.0);
Buffer* vb = Buffer<float<4>[]>.CreateFromArray(VERTEX | COPY_DST, verts);
class UniformData {
  float opacity;
}
class PipelineData {
  void vertexShader(VertexBuiltins vb, float<4> v) vertex { vb.position = v; }
  float<4> fragmentShader(FragmentBuiltins fb, float<4> v) fragment {
    return float<4>(0.0, 1.0, 0.0, 1.0) * uniforms.opacity;
  }
  uniform UniformData* uniforms;
}
auto pipeline = new RenderPipeline<PipelineData>(device, null, TriangleList);
UniformBuffer<UniformData>* uniformBuffer = device.CreateUniformBuffer<UniformData>({ opacity : 0.5 });
PipelineData* pipelineData = new PipelineData(uniforms : uniformBuffer);
while (System.IsRunning()) {
  System.GetNextEvent();
  renderable Texture2DView* framebuffer = swapChain.GetCurrentTextureView();
  CommandEncoder* encoder = new CommandEncoder(device);
  RenderPassEncoder* passEncoder = encoder.BeginRenderPass(framebuffer);
  passEncoder.SetPipeline(pipeline);
  passEncoder.SetPipelineData(pipelineData);
  passEncoder.SetVertexBuffer(0, vb);
  passEncoder.Draw(3, 1, 0, 0);
  passEncoder.End();
  CommandBuffer* cb = encoder.Finish();
  queue.Submit(cb);
  swapChain.Present();
}
return 0.0;
