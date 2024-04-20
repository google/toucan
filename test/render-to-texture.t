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
  float<4> fragmentShader(FragmentBuiltins fb) fragment { return float<4>(0.0, 1.0, 0.0, 1.0); }
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
    float<4> fragmentShader(FragmentBuiltins fb, float<2> texCoord) fragment {
      return textureView.Sample(sampler, texCoord);
    }
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

// FIXME: we have to use PreferredSwapChainFormat because color attachments don't exist yet
auto tex = new sampleable renderable Texture2D<PreferredSwapChainFormat>(device, 640, 480);
RenderPipeline* triPipeline = new RenderPipeline<GreenPipeline>(device, null, TriangleList);
CommandEncoder* encoder = new CommandEncoder(device);
RenderPassEncoder* drawEncoder = encoder.BeginRenderPass(tex);
drawEncoder.SetPipeline(triPipeline);
drawEncoder.SetVertexBuffer(0, triVB);
drawEncoder.Draw(3, 1, 0, 0);
drawEncoder.End();

auto framebuffer = swapChain.GetCurrentTexture().CreateRenderableView();
RenderPipeline* quadPipeline = new RenderPipeline<TexPipeline>(device, null, TriangleList);
auto samplerBG = new BindGroup(device, sampler);
auto texView = tex.CreateSampleableView();
auto texBG = new BindGroup(device, texView);
RenderPassEncoder* passEncoder = encoder.BeginRenderPass(framebuffer);
passEncoder.SetPipeline(quadPipeline);
passEncoder.SetBindGroup(0, samplerBG);
passEncoder.SetBindGroup(1, texBG);
passEncoder.SetVertexBuffer(0, quadVB);
passEncoder.SetIndexBuffer(quadIB);
passEncoder.DrawIndexed(6, 1, 0, 0, 0);
passEncoder.End();

device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
return 0.0;
