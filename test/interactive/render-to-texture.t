var device = new Device();
var window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var triVerts : float<4>[3] = { { 0.0,  1.0, 0.0, 1.0 }, {-1.0, -1.0, 0.0, 1.0 }, { 1.0, -1.0, 0.0, 1.0 }};
var triVB = new vertex Buffer<float<4>[]>(device, &triVerts);
class GreenPipeline {
    void vertexShader(VertexBuiltins^ vb) vertex { vb.position = position.Get(); }
    void fragmentShader(FragmentBuiltins^ fb) fragment { renderTex.Set(float<4>(0.0, 1.0, 0.0, 1.0)); }
    var position : vertex Buffer<float<4>[]>*;
    var renderTex : ColorAttachment<RGBA8unorm>*;
}

class QuadVertex {
    var position : float<4>;
    var texCoord : float<2>;
};

class Bindings {
    var sampler : Sampler*;
    var textureView : SampleableTexture2D<float>*;
}

class TexPipeline {
    float<2> vertexShader(VertexBuiltins^ vb) vertex {
        var v = vertices.Get();
        vb.position = v.position;
        return v.texCoord;
    }
    void fragmentShader(FragmentBuiltins^ fb, float<2> texCoord) fragment {
      var b = bindings.Get();
      fragColor.Set(b.textureView.Sample(b.sampler, texCoord));
    }
    var vertices : vertex Buffer<QuadVertex[]>*;
    var indices : index Buffer<uint[]>*;
    var fragColor : ColorAttachment<PreferredSwapChainFormat>*;
    var bindings : BindGroup<Bindings>*;
};

var quadVerts : QuadVertex[4] = {
  { position = { -1.0, -1.0, 0.0, 1.0 }, texCoord = { 0.0, 1.0 } },
  { position = {  1.0, -1.0, 0.0, 1.0 }, texCoord = { 1.0, 1.0 } },
  { position = { -1.0,  1.0, 0.0, 1.0 }, texCoord = { 0.0, 0.0 } },
  { position = {  1.0,  1.0, 0.0, 1.0 }, texCoord = { 1.0, 0.0 } }
};
var quadIndices : uint[6] = { 0, 1, 2, 1, 2, 3 };
var quadVB = new vertex Buffer<QuadVertex[]>(device, &quadVerts);
var quadIB = new index Buffer<uint[]>(device, &quadIndices);
var sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);

var tex = new sampleable renderable Texture2D<RGBA8unorm>(device, window.GetSize());
var triPipeline = new RenderPipeline<GreenPipeline>(device, null, TriangleList);
var encoder = new CommandEncoder(device);
var gp : GreenPipeline;
gp.position = triVB;
gp.renderTex = new ColorAttachment<RGBA8unorm>(tex, Clear, Store);
var renderPass = new RenderPass<GreenPipeline>(encoder, &gp);
renderPass.SetPipeline(triPipeline);
renderPass.Draw(3, 1, 0, 0);
renderPass.End();

var quadPipeline = new RenderPipeline<TexPipeline>(device, null, TriangleList);
var texView = tex.CreateSampleableView();
var fb = new ColorAttachment<PreferredSwapChainFormat>(swapChain.GetCurrentTexture(), Clear, Store);
var quadBG = new BindGroup<Bindings>(device, { sampler = sampler, textureView = texView} );
var drawPass = new RenderPass<TexPipeline>(encoder,
  { fragColor = fb, vertices = quadVB, indices = quadIB, bindings = quadBG }
);
drawPass.SetPipeline(quadPipeline);
drawPass.DrawIndexed(6, 1, 0, 0, 0);
drawPass.End();

device.GetQueue().Submit(encoder.Finish());
swapChain.Present();
while (System.IsRunning()) System.GetNextEvent();
