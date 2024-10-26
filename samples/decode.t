class Vertex {
    var position : float<4>;
    var texCoord : float<2>;
};

var device = new Device();

using Format = RGBA8unorm;
var image = new ImageDecoder<RGBA8unorm>(inline("third_party/libjpeg-turbo/testimages/testorig.jpg"));
var imageSize = image.GetSize();
var texture = new sampleable Texture2D<Format>(device, imageSize);
var buffer = new writeonly Buffer<[]Format::HostType>(device, texture.MinBufferWidth() * imageSize.y);
var b = buffer.Map();
image.Decode(b, texture.MinBufferWidth());
buffer.Unmap();
var copyEncoder = new CommandEncoder(device);
texture.CopyFromBuffer(copyEncoder, buffer, imageSize);
device.GetQueue().Submit(copyEncoder.Finish());

var window = new Window({0, 0}, imageSize);
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = [4] new Vertex;
verts[0].position = float<4>(-1.0,  1.0, 0.0, 1.0);
verts[1].position = float<4>( 1.0,  1.0, 0.0, 1.0);
verts[2].position = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[3].position = float<4>( 1.0, -1.0, 0.0, 1.0);
verts[0].texCoord = float<2>(0.0, 0.0);
verts[1].texCoord = float<2>(1.0, 0.0);
verts[2].texCoord = float<2>(0.0, 1.0);
verts[3].texCoord = float<2>(1.0, 1.0);
var indices = [6] new uint;
indices[0] = 0;
indices[1] = 1;
indices[2] = 2;
indices[3] = 1;
indices[4] = 2;
indices[5] = 3;
var vb = new vertex Buffer<[]Vertex>(device, verts);
var ib = new index Buffer<[]uint>(device, indices);
class Bindings {
  var sampler : *Sampler;
  var textureView : *SampleableTexture2D<float>;
}

class Pipeline {
    vertex main(vb : ^VertexBuiltins) : float<2> {
        var v = vert.Get();
        vb.position = v.position;
        return v.texCoord;
    }
    fragment main(fb : ^FragmentBuiltins, texCoord : float<2>) {
      fragColor.Set(bindings.Get().textureView.Sample(bindings.Get().sampler, texCoord));
    }
    var vert : *vertex Buffer<[]Vertex>;
    var indexBuffer : *index Buffer<[]uint>;
    var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
    var bindings : *BindGroup<Bindings>;
};
var pipeline = new RenderPipeline<Pipeline>(device, null, TriangleList);
var sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);
var texView = texture.CreateSampleableView();
var bindings : Bindings;
bindings.sampler = sampler;
bindings.textureView = texView;
var bindGroup = new BindGroup<Bindings>(device, &bindings);
var encoder = new CommandEncoder(device);
var p : Pipeline;
p.vert = vb;
p.indexBuffer = ib;
p.fragColor = swapChain.GetCurrentTexture().CreateColorAttachment(Clear, Store);
p.bindings = bindGroup;
var renderPass = new RenderPass<Pipeline>(encoder, &p);
renderPass.SetPipeline(pipeline);
renderPass.DrawIndexed(6, 1, 0, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
