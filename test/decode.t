class Vertex {
    float<4> position;
    float<2> texCoord;
};
class Varyings {
    float<2> texCoord;
};

Device* device = new Device();

using Format = RGBA8unorm;
auto image = new ImageDecoder<RGBA8unorm>(inline("third_party/libjpeg-turbo/testimages/testorig.jpg"));
auto texture = new sampleable Texture2D<Format>(device, image.Width(), image.Height(), 1);
auto buffer = new Buffer<Format::MemoryType[]>(device, texture.MinBufferWidth() * image.Height());
writeonly Format::MemoryType[]^ b = buffer.MapWrite();
image.Decode(b, texture.MinBufferWidth());
buffer.Unmap();
CommandEncoder* copyEncoder = new CommandEncoder(device);
texture.CopyFromBuffer(copyEncoder, buffer, image.Width(), image.Height(), 1, uint<3>(0, 0, 0));
device.GetQueue().Submit(copyEncoder.Finish());

Window* window = new Window(device, 0, 0, image.Width(), image.Height());
SwapChain* swapChain = new SwapChain(window);
auto verts = new Vertex[4];
verts[0].position = float<4>(-1.0,  1.0, 0.0, 1.0);
verts[1].position = float<4>( 1.0,  1.0, 0.0, 1.0);
verts[2].position = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[3].position = float<4>( 1.0, -1.0, 0.0, 1.0);
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
    SampleableTexture2D<float>* textureView;
};
RenderPipeline* pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
auto sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);
auto samplerBG = new BindGroup(device, sampler);
auto texView = texture.CreateSampleableView();
auto texBG = new BindGroup(device, texView);
renderable SampleableTexture2D* framebuffer = swapChain.GetCurrentTextureView();
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
