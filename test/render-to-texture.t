Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);
auto triVerts = new float<4>[3];
triVerts[0] = float<4>( 0.0,  1.0, 0.0, 1.0);
triVerts[1] = float<4>(-1.0, -1.0, 0.0, 1.0);
triVerts[2] = float<4>( 1.0, -1.0, 0.0, 1.0);
auto triVB = new vertex Buffer<float<4>[]>(device, triVerts);
class GreenPipeline {
    void vertexShader(VertexBuiltins vb, float<4> v) vertex { vb.position = v; }
    void fragmentShader(FragmentBuiltins fb) fragment { renderTex.Set(float<4>(0.0, 1.0, 0.0, 1.0)); }

    ColorAttachment<RGBA8unorm>* renderTex;
}

class QuadVertex {
    float<4> position;
    float<2> texCoord;
};
class TexPipeline {
    float<2> vertexShader(VertexBuiltins vb, QuadVertex v) vertex {
        vb.position = v.position;
        return v.texCoord;
    }
    void fragmentShader(FragmentBuiltins fb, float<2> texCoord) fragment {
      fragColor.Set(textureView.Sample(sampler, texCoord));
    }
    ColorAttachment<PreferredSwapChainFormat>* fragColor;
    Sampler* sampler;
    SampleableTexture2D<float>* textureView;
};

auto quadVerts = new QuadVertex[4];
quadVerts[0].position = float<4>(-1.0, -1.0, 0.0, 1.0);
quadVerts[1].position = float<4>( 1.0, -1.0, 0.0, 1.0);
quadVerts[2].position = float<4>(-1.0,  1.0, 0.0, 1.0);
quadVerts[3].position = float<4>( 1.0,  1.0, 0.0, 1.0);
quadVerts[0].texCoord = float<2>(0.0, 1.0);
quadVerts[1].texCoord = float<2>(1.0, 1.0);
quadVerts[2].texCoord = float<2>(0.0, 0.0);
quadVerts[3].texCoord = float<2>(1.0, 0.0);
auto quadIndices = new int[6];
quadIndices[0] = 0;
quadIndices[1] = 1;
quadIndices[2] = 2;
quadIndices[3] = 1;
quadIndices[4] = 2;
quadIndices[5] = 3;
auto quadVB = new vertex Buffer<QuadVertex[]>(device, quadVerts);
auto quadIB = new index Buffer<int[]>(device, quadIndices);
auto sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);

auto tex = new sampleable renderable Texture2D<RGBA8unorm>(device, 640, 480);
RenderPipeline* triPipeline = new RenderPipeline<GreenPipeline>(device, null, TriangleList);
auto encoder = new CommandEncoder(device);
GreenPipeline gp;
gp.renderTex = new ColorAttachment<RGBA8unorm>(tex, Clear, Store);
auto renderPass = new RenderPass<GreenPipeline>(encoder, &gp);
renderPass.SetPipeline(triPipeline);
renderPass.SetVertexBuffer(0, triVB);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();

RenderPipeline* quadPipeline = new RenderPipeline<TexPipeline>(device, null, TriangleList);
auto samplerBG = new BindGroup(device, sampler);
auto texView = tex.CreateSampleableView();
auto texBG = new BindGroup(device, texView);
TexPipeline t;
t.fragColor = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
auto drawPass = new RenderPass<TexPipeline>(encoder, &t);
drawPass.SetPipeline(quadPipeline);
drawPass.SetBindGroup(0, samplerBG);
drawPass.SetBindGroup(1, texBG);
drawPass.SetVertexBuffer(0, quadVB);
drawPass.SetIndexBuffer(quadIB);
drawPass.DrawIndexed(6, 1, 0, 0, 0);
drawPass.End();

device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
return 0.0;
