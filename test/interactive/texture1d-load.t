class Vertex {
  var position : float<2>;
  var texCoord : float;
};

class Bindings {
  var textureView : *SampleableTexture1D<float>;
}

class Pipeline {
    vertex main(vb : &VertexBuiltins) : float {
        var v = vertices.Get();
        vb.position = {@v.position, 0.0, 1.0};
        return v.texCoord;
    }
    fragment main(fb : &FragmentBuiltins, texCoord : float) {
      var b = bindings.Get();
      var coord = (texCoord * 2.0) as int;
      fragColor.Set(b.textureView.Load(coord, 0));
    }
    var vertices : *VertexInput<Vertex>;
    var indices : *index Buffer<[]uint>;
    var fragColor : *ColorOutput<PreferredPixelFormat>;
    var bindings : *BindGroup<Bindings>;
};
var device = new Device();
var window = new Window({640, 480});
var swapChain = new SwapChain<PreferredPixelFormat>(device, window);
var verts = [4]Vertex{
  { position = {-1.0,  1.0}, texCoord = 0.0 },
  { position = { 1.0,  1.0}, texCoord = 1.0 },
  { position = {-1.0, -1.0}, texCoord = 0.0 },
  { position = { 1.0, -1.0}, texCoord = 1.0 }
};
var indices = [6]uint{ 0, 1, 2, 1, 2, 3 };
var vb = new vertex Buffer<[]Vertex>(device, &verts);
var pipeline = new RenderPipeline<Pipeline>(device);
var tex = new sampleable Texture1D<RGBA8unorm>(device, 2);
var buffer = new hostwriteable Buffer<[]ubyte<4>>(device, 2);
{
  var data = buffer.MapWrite();
  data[0] = ubyte<4>(255ub,   0ub,   0ub, 255ub);
  data[1] = ubyte<4>(  0ub, 255ub,   0ub, 255ub);
}
var copyEncoder = new CommandEncoder(device);
tex.CopyFromBuffer(copyEncoder, buffer, 2);
device.GetQueue().Submit(copyEncoder.Finish());
var bindGroup = new BindGroup<Bindings>(device, {
  textureView = tex.CreateSampleableView()
});

var encoder = new CommandEncoder(device);
var p = Pipeline{
  vertices = new VertexInput<Vertex>(vb),
  indices = new index Buffer<[]uint>(device, &indices),
  fragColor = swapChain.GetCurrentTexture().CreateColorOutput(LoadOp.Clear),
  bindings = bindGroup
};
var renderPass = new RenderPass<Pipeline>(encoder, &p);
renderPass.SetPipeline(pipeline);
renderPass.DrawIndexed(6, 1, 0, 0, 0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
