class Vertex {
    float<4> position;
    float<2> texCoord;
};

Device* device = new Device();

using Format = RGBA8unorm;
auto image = new ImageDecoder<RGBA8unorm>(inline("third_party/libjpeg-turbo/testimages/testorig.jpg"));
auto imageSize = image.GetSize();
auto texture = new sampleable Texture2D<Format>(device, imageSize);
auto buffer = new Buffer<Format::MemoryType[]>(device, texture.MinBufferWidth() * imageSize.y);
writeonly Format::MemoryType[]^ b = buffer.MapWrite();
image.Decode(b, texture.MinBufferWidth());
buffer.Unmap();
auto copyEncoder = new CommandEncoder(device);
texture.CopyFromBuffer(copyEncoder, buffer, imageSize);
device.GetQueue().Submit(copyEncoder.Finish());

Window* window = new Window({0, 0}, imageSize);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
auto verts = new Vertex[4];
verts[0].position = float<4>(-1.0,  1.0, 0.0, 1.0);
verts[1].position = float<4>( 1.0,  1.0, 0.0, 1.0);
verts[2].position = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[3].position = float<4>( 1.0, -1.0, 0.0, 1.0);
verts[0].texCoord = float<2>(0.0, 0.0);
verts[1].texCoord = float<2>(1.0, 0.0);
verts[2].texCoord = float<2>(0.0, 1.0);
verts[3].texCoord = float<2>(1.0, 1.0);
auto indices = new uint[6];
indices[0] = 0;
indices[1] = 1;
indices[2] = 2;
indices[3] = 1;
indices[4] = 2;
indices[5] = 3;
auto vb = new vertex Buffer<Vertex[]>(device, verts);
auto ib = new index Buffer<uint[]>(device, indices);
class Bindings {
  Sampler* sampler;
  SampleableTexture2D<float>* textureView;
}

class Pipeline {
    float<2> vertexShader(VertexBuiltins vb) vertex {
        Vertex v = vert.Get();
        vb.position = v.position;
        return v.texCoord;
    }
    void fragmentShader(FragmentBuiltins fb, float<2> texCoord) fragment {
      fragColor.Set(bindings.Get().textureView.Sample(bindings.Get().sampler, texCoord));
    }
    vertex Buffer<Vertex[]>* vert;
    index Buffer<uint[]>* indexBuffer;
    ColorAttachment<PreferredSwapChainFormat>* fragColor;
    BindGroup<Bindings>* bindings;
};
auto pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
auto sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);
auto texView = texture.CreateSampleableView();
Bindings bindings;
bindings.sampler = sampler;
bindings.textureView = texView;
auto bindGroup = new BindGroup<Bindings>(device, &bindings);
auto encoder = new CommandEncoder(device);
Pipeline p;
p.vert = vb;
p.indexBuffer = ib;
p.fragColor = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
p.bindings = bindGroup;
auto renderPass = new RenderPass<Pipeline>(encoder, &p);
renderPass.SetPipeline(pipeline);
renderPass.DrawIndexed(6, 1, 0, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
