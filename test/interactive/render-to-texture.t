Device* device = new Device();
Window* window = new Window(0, 0, 640, 480);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
float<4>[3] triVerts = { { 0.0,  1.0, 0.0, 1.0 }, {-1.0, -1.0, 0.0, 1.0 }, { 1.0, -1.0, 0.0, 1.0 }};
auto triVB = new vertex Buffer<float<4>[]>(device, &triVerts);
class GreenPipeline {
    void vertexShader(VertexBuiltins vb) vertex { vb.position = position.Get(); }
    void fragmentShader(FragmentBuiltins fb) fragment { renderTex.Set(float<4>(0.0, 1.0, 0.0, 1.0)); }
    vertex Buffer<float<4>[]>* position;
    ColorAttachment<RGBA8unorm>* renderTex;
}

class QuadVertex {
    float<4> position;
    float<2> texCoord;
};

class Bindings {
    Sampler* sampler;
    SampleableTexture2D<float>* textureView;
}

class TexPipeline {
    float<2> vertexShader(VertexBuiltins vb) vertex {
        QuadVertex v = vertices.Get();
        vb.position = v.position;
        return v.texCoord;
    }
    void fragmentShader(FragmentBuiltins fb, float<2> texCoord) fragment {
      auto b = bindings.Get();
      fragColor.Set(b.textureView.Sample(b.sampler, texCoord));
    }
    vertex Buffer<QuadVertex[]>* vertices;
    index Buffer<uint[]>* indices;
    ColorAttachment<PreferredSwapChainFormat>* fragColor;
    BindGroup<Bindings>* bindings;
};

QuadVertex[4] quadVerts = {
  { position = { -1.0, -1.0, 0.0, 1.0 }, texCoord = { 0.0, 1.0 } },
  { position = {  1.0, -1.0, 0.0, 1.0 }, texCoord = { 1.0, 1.0 } },
  { position = { -1.0,  1.0, 0.0, 1.0 }, texCoord = { 0.0, 0.0 } },
  { position = {  1.0,  1.0, 0.0, 1.0 }, texCoord = { 1.0, 0.0 } }
};
uint[6] quadIndices = { 0, 1, 2, 1, 2, 3 };
auto quadVB = new vertex Buffer<QuadVertex[]>(device, &quadVerts);
auto quadIB = new index Buffer<uint[]>(device, &quadIndices);
auto sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);

auto tex = new sampleable renderable Texture2D<RGBA8unorm>(device, 640, 480);
auto triPipeline = new RenderPipeline<GreenPipeline>(device, null, TriangleList);
auto encoder = new CommandEncoder(device);
GreenPipeline gp;
gp.position = triVB;
gp.renderTex = new ColorAttachment<RGBA8unorm>(tex, Clear, Store);
auto renderPass = new RenderPass<GreenPipeline>(encoder, &gp);
renderPass.SetPipeline(triPipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();

auto quadPipeline = new RenderPipeline<TexPipeline>(device, null, TriangleList);
auto texView = tex.CreateSampleableView();
auto fb = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
auto quadBG = new BindGroup<Bindings>(device, { sampler = sampler, textureView = texView} );
auto drawPass = new RenderPass<TexPipeline>(encoder,
  { fragColor = fb, vertices = quadVB, indices = quadIB, bindings = quadBG }
);
drawPass.SetPipeline(quadPipeline);
drawPass.DrawIndexed(6, 1, 0, 0, 0);
drawPass.End();

device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
