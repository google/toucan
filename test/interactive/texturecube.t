class Vertex {
    float<4> position;
    float<3> texCoord;
};
class Varyings {
    float<3> texCoord;
};

Device* device = new Device();
Window* window = new Window({0, 0}, {640, 480});
auto swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
auto verts = new Vertex[4];
verts[0].position = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[1].position = float<4>( 1.0, -1.0, 0.0, 1.0);
verts[2].position = float<4>(-1.0,  1.0, 0.0, 1.0);
verts[3].position = float<4>( 1.0,  1.0, 0.0, 1.0);
verts[0].texCoord = float<3>(1.0, 0.0, 0.0);
verts[1].texCoord = float<3>(0.0, 1.0, 0.0);
verts[2].texCoord = float<3>(0.0, 0.0, 1.0);
verts[3].texCoord = float<3>(1.0, 1.0, 0.0);
auto indices = new uint[6];
indices[0] = 0;
indices[1] = 1;
indices[2] = 2;
indices[3] = 1;
indices[4] = 2;
indices[5] = 3;

class Bindings {
  Sampler* sampler;
  SampleableTextureCube<float>* textureView;
}

class Pipeline {
    Varyings vertexShader(VertexBuiltins vb) vertex {
        Vertex v = vert.Get();
        vb.position = v.position;
        Varyings varyings;
        varyings.texCoord = v.texCoord;
        return varyings;
    }
    void fragmentShader(FragmentBuiltins fb, Varyings varyings) fragment {
      auto b = bindings.Get();
      fragColor.Set(b.textureView.Sample(b.sampler, varyings.texCoord));
    }
    vertex Buffer<Vertex[]>* vert;
    index Buffer<uint[]>*    indices;
    ColorAttachment<PreferredSwapChainFormat>* fragColor;
    BindGroup<Bindings>*     bindings;
};
auto pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
auto tex = new sampleable TextureCube<RGBA8unorm>(device, {2, 2});
auto buffer = new Buffer<ubyte<4>[]>(device, 64 * 2 * 2);
auto data = buffer.MapWrite();
data[0]   =  ubyte<4>(255ub,   0ub,   0ub, 255ub);
data[1]   =  ubyte<4>(  0ub, 255ub,   0ub, 255ub);
data[64]  =  ubyte<4>(  0ub,   0ub, 255ub, 255ub);
data[65]  =  ubyte<4>(  0ub, 255ub, 255ub, 255ub);
data[128] =  ubyte<4>(255ub, 255ub,   0ub, 255ub);
data[129] =  ubyte<4>(  0ub, 255ub, 255ub, 255ub);
data[192]  = ubyte<4>(255ub,   0ub, 255ub, 255ub);
data[193]  = ubyte<4>(255ub, 255ub, 255ub, 255ub);
buffer.Unmap();
auto copyEncoder = new CommandEncoder(device);
tex.CopyFromBuffer(copyEncoder, buffer, {2, 2, 2});
device.GetQueue().Submit(copyEncoder.Finish());
Bindings bindings;
bindings.sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);
bindings.textureView = tex.CreateSampleableView();
auto bindGroup = new BindGroup<Bindings>(device, &bindings);

auto encoder = new CommandEncoder(device);
Pipeline p;
p.fragColor = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
p.vert = new vertex Buffer<Vertex[]>(device, verts);
p.indices = new index Buffer<uint[]>(device, indices);
p.bindings = bindGroup;
auto renderPass = new RenderPass<Pipeline>(encoder, &p);
renderPass.SetPipeline(pipeline);
renderPass.DrawIndexed(6, 1, 0, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
