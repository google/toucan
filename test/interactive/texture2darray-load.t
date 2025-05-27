class Vertex {
    var position : float<4>;
    var texCoord : float<2>;
};
class Varyings {
    var texCoord : float<2>;
};

var device = new Device();
var window = new Window({0, 0}, {640, 480});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var verts = [4] new Vertex;
verts[0].position = float<4>(-1.0, -1.0, 0.0, 1.0);
verts[1].position = float<4>( 1.0, -1.0, 0.0, 1.0);
verts[2].position = float<4>(-1.0,  1.0, 0.0, 1.0);
verts[3].position = float<4>( 1.0,  1.0, 0.0, 1.0);
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

class Bindings {
  var textureView : *SampleableTexture2DArray<float>;
}

class Pipeline {
    vertex main(vb : &VertexBuiltins) : Varyings {
        var v = vert.Get();
        vb.position = v.position;
        var varyings : Varyings;
        varyings.texCoord = v.texCoord;
        return varyings;
    }
    fragment main(fb : &FragmentBuiltins, varyings : Varyings) {
      var b = bindings.Get();
      var coord = int<2>((int) (varyings.texCoord.x * 2.0), (int) (varyings.texCoord.y * 2.0));
      fragColor.Set(b.textureView.Load(coord, 1, 0));
    }
    var vert : *VertexInput<Vertex>;
    var indices : *index Buffer<[]uint>;
    var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
    var bindings : *BindGroup<Bindings>;
};
var pipeline = new RenderPipeline<Pipeline>(device);
var tex = new sampleable Texture2DArray<RGBA8unorm>(device, {2, 2, 2});
var buffer = new hostwriteable Buffer<[]ubyte<4>>(device, 64 * 2 * 2);
var data = buffer.Map();
data[0]   =  ubyte<4>(255ub,   0ub,   0ub, 255ub);
data[1]   =  ubyte<4>(  0ub, 255ub,   0ub, 255ub);
data[64]  =  ubyte<4>(  0ub,   0ub, 255ub, 255ub);
data[65]  =  ubyte<4>(  0ub, 255ub, 255ub, 255ub);
data[128] =  ubyte<4>(255ub, 255ub,   0ub, 255ub);
data[129] =  ubyte<4>(  0ub, 255ub, 255ub, 255ub);
data[192]  = ubyte<4>(255ub,   0ub, 255ub, 255ub);
data[193]  = ubyte<4>(255ub, 255ub, 255ub, 255ub);
buffer.Unmap();
var copyEncoder = new CommandEncoder(device);
tex.CopyFromBuffer(copyEncoder, buffer, {2, 2, 2});
device.GetQueue().Submit(copyEncoder.Finish());
var bindings : Bindings;
bindings.textureView = tex.CreateSampleableView();
var bindGroup = new BindGroup<Bindings>(device, &bindings);

var encoder = new CommandEncoder(device);
var p : Pipeline;
p.fragColor = swapChain.GetCurrentTexture().CreateColorAttachment(LoadOp.Clear);
var vb = new vertex Buffer<[]Vertex>(device, verts);
p.vert = new VertexInput<Vertex>(vb);
p.indices = new index Buffer<[]uint>(device, indices);
p.bindings = bindGroup;
var renderPass = new RenderPass<Pipeline>(encoder, &p);
renderPass.SetPipeline(pipeline);
renderPass.DrawIndexed(6, 1, 0, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
