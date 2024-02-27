class Vertex {
    float<4> position;
    float<3> texCoord;
};
class Varyings {
    float<3> texCoord;
};

Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
SwapChain* swapChain = new SwapChain(window);
auto verts = new Vertex[4];
verts[0].position = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[1].position = float<4>( 1.0, -1.0, 0.0, 1.0);
verts[2].position = float<4>(-1.0,  1.0, 0.0, 1.0);
verts[3].position = float<4>( 1.0,  1.0, 0.0, 1.0);
verts[0].texCoord = float<3>(1.0, 0.0, 0.0);
verts[1].texCoord = float<3>(0.0, 1.0, 0.0);
verts[2].texCoord = float<3>(0.0, 0.0, 1.0);
verts[3].texCoord = float<3>(1.0, 1.0, 0.0);
auto indices = new int[6];
indices[0] = 0;
indices[1] = 1;
indices[2] = 2;
indices[3] = 1;
indices[4] = 2;
indices[5] = 3;
auto vb = new vertex Buffer<Vertex[]>(device, verts.length);
vb.SetData(verts);
auto ib = new index Buffer<int[]>(device, 6);
ib.SetData(indices);
class Pipeline {
    Varyings vertexShader(VertexBuiltins vb, Vertex v) vertex {
        vb.position = v.position;
        Varyings varyings;
        varyings.texCoord = v.texCoord;
        return varyings;
    }
    float<4> fragmentShader(FragmentBuiltins fb, Varyings varyings) fragment {
      return textureView.Sample(sampler, varyings.texCoord, 0);
    }
    Sampler* sampler;
    sampled TextureCubeArrayView<float>* textureView;
};
RenderPipeline* pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
auto sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);
auto width = 2;
auto height = 2;
auto layers = 12;
auto tex = new sampled Texture2D<RGBA8unorm>(device, width, height, layers);
auto bufferWidth = tex.MinBufferWidth();
auto buffer = new Buffer<ubyte<4>[]>(device, bufferWidth * height * layers);
auto data = buffer.MapWrite();
ubyte<4> value = ubyte<4>(0ub, 128ub, 255ub, 255ub);
for (int layer = 0; layer < layers; ++layer) {
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      data[(layer * height + y) * bufferWidth + x] = value;
      value += ubyte<4>(64ub, 32ub, 240ub, 0ub);
    }
  }
}
buffer.Unmap();
CommandEncoder* copyEncoder = new CommandEncoder(device);
tex.CopyFromBuffer(copyEncoder, buffer, 2, 2, 2, uint<3>(0, 0, 0));
device.GetQueue().Submit(copyEncoder.Finish());
auto samplerBG = new BindGroup(device, sampler);
auto texView = tex.CreateSampledCubeArrayView();
auto texBG = new BindGroup(device, texView);
renderable Texture2DView* framebuffer = swapChain.GetCurrentTextureView();
CommandEncoder* encoder = new CommandEncoder(device);
RenderPassEncoder* passEncoder = encoder.BeginRenderPass(framebuffer);
passEncoder.SetPipeline(pipeline);
passEncoder.SetBindGroup(0, samplerBG);
passEncoder.SetBindGroup(1, texBG);
passEncoder.SetVertexBuffer(0, vb);
passEncoder.SetIndexBuffer(ib);
passEncoder.DrawIndexed(6, 1, 0, 0, 0);
passEncoder.End();
CommandBuffer* cb = encoder.Finish();
device.GetQueue().Submit(cb);
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
return 0.0;
