class Vertex {
    float<4> position;
    float<2> texCoord;
};
class Varyings {
    float<2> texCoord;
};

Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
SwapChain* swapChain = new SwapChain(window);
auto verts = new Vertex[4];
verts[0].position = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[1].position = float<4>( 1.0, -1.0, 0.0, 1.0);
verts[2].position = float<4>(-1.0,  1.0, 0.0, 1.0);
verts[3].position = float<4>( 1.0,  1.0, 0.0, 1.0);
verts[0].texCoord = float<2>(0.0, 0.0);
verts[1].texCoord = float<2>(1.0, 0.0);
verts[2].texCoord = float<2>(0.0, 1.0);
verts[3].texCoord = float<2>(1.0, 1.0);
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
      return textureView.Sample(sampler, varyings.texCoord);
    }
    Sampler* sampler;
    sampled Texture2DView<float>* textureView;
};
RenderPipeline* pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
auto sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);
auto tex = new sampled Texture2D<RGBA8unorm>(device, 2, 2, 1);
auto width = tex.MinBufferWidth();
auto buffer = new Buffer<ubyte<4>[]>(device, 2 * width);
auto data = buffer.MapWrite();
data[0] =         ubyte<4>(255ub,   0ub,   0ub, 255ub);
data[1] =         ubyte<4>(  0ub, 255ub,   0ub, 255ub);
data[width    ] = ubyte<4>(  0ub,   0ub, 255ub, 255ub);
data[width + 1] = ubyte<4>(  0ub, 255ub, 255ub, 255ub);
buffer.Unmap();
CommandEncoder* encoder = new CommandEncoder(device);
tex.CopyFromBuffer(encoder, buffer, 2, 2, 1, uint<3>(0, 0, 0));
device.GetQueue().Submit(encoder.Finish());
auto samplerBG = new BindGroup(device, sampler);
auto texView = tex.CreateSampledView();
auto texBG = new BindGroup(device, texView);
while (System.IsRunning()) {
  System.GetNextEvent();
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
}
return 0.0;
