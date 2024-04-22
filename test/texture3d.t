class Vertex {
    float<4> position;
    float<3> texCoord;
};
class Varyings {
    float<3> texCoord;
};

Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);
auto verts = new Vertex[4];
verts[0].position = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[1].position = float<4>( 1.0, -1.0, 0.0, 1.0);
verts[2].position = float<4>(-1.0,  1.0, 0.0, 1.0);
verts[3].position = float<4>( 1.0,  1.0, 0.0, 1.0);
verts[0].texCoord = float<3>(0.0, 0.0, 0.5);
verts[1].texCoord = float<3>(1.0, 0.0, 0.5);
verts[2].texCoord = float<3>(0.0, 1.0, 0.5);
verts[3].texCoord = float<3>(1.0, 1.0, 0.5);
auto indices = new int[6];
indices[0] = 0;
indices[1] = 1;
indices[2] = 2;
indices[3] = 1;
indices[4] = 2;
indices[5] = 3;
auto vb = new vertex Buffer<Vertex[]>(device, verts);
auto ib = new index Buffer<int[]>(device, indices);
class Pipeline {
    Varyings vertexShader(VertexBuiltins vb, Vertex v) vertex {
        vb.position = v.position;
        Varyings varyings;
        varyings.texCoord = v.texCoord;
        return varyings;
    }
    void fragmentShader(FragmentBuiltins fb, Varyings varyings) fragment {
      fragColor.Set(textureView.Sample(sampler, varyings.texCoord));
    }
    Sampler* sampler;
    SampleableTexture3D<float>* textureView;
    ColorAttachment<PreferredSwapChainFormat>* fragColor;
};
RenderPipeline* pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
auto sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);
auto tex = new sampleable Texture3D<RGBA8unorm>(device, 2, 2, 2);
auto buffer = new Buffer<ubyte<4>[]>(device, 64 * 2 * 2);
auto data = buffer.MapWrite();
data[0]   =  ubyte<4>(255ub,   0ub,   0ub, 255ub);
data[1]   =  ubyte<4>(  0ub, 255ub,   0ub, 255ub);
data[64]  =  ubyte<4>(  0ub,   0ub, 255ub, 255ub);
data[65]  =  ubyte<4>(  0ub, 255ub, 255ub, 255ub);
data[128] =  ubyte<4>(  0ub, 255ub,   0ub, 255ub);
data[129] =  ubyte<4>(  0ub,   0ub, 255ub, 255ub);
data[192]  = ubyte<4>(255ub,   0ub,   0ub, 255ub);
data[193]  = ubyte<4>(255ub, 255ub, 255ub, 255ub);
buffer.Unmap();
auto copyEncoder = new CommandEncoder(device);
tex.CopyFromBuffer(copyEncoder, buffer, 2, 2, 2);
device.GetQueue().Submit(copyEncoder.Finish());
auto samplerBG = new BindGroup(device, sampler);
auto texView = tex.CreateSampleableView();
auto texBG = new BindGroup(device, texView);

auto encoder = new CommandEncoder(device);
Pipeline p;
p.fragColor = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
auto renderPass = new RenderPass<Pipeline>(encoder, &p);
renderPass.SetPipeline(pipeline);
renderPass.SetBindGroup(0, samplerBG);
renderPass.SetBindGroup(1, texBG);
renderPass.SetVertexBuffer(0, vb);
renderPass.SetIndexBuffer(ib);
renderPass.DrawIndexed(6, 1, 0, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
return 0.0;
