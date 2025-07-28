class Vertex {
  var position : float<2>;
  var texCoord : float<2>;
};

class Bindings {
  var sampler : *Sampler;
  var textureView : *SampleableTexture2D<float>;
}

class Pipeline {
    vertex main(vb : &VertexBuiltins) : float<2> {
        var v = vertices.Get();
        vb.position = {@v.position, 0.0, 1.0};
        return v.texCoord;
    }
    fragment main(fb : &FragmentBuiltins, texCoord : float<2>) {
      fragColor.Set(bindings.Get().textureView.Sample(bindings.Get().sampler, texCoord));
    }
    var vertices : *VertexInput<Vertex>;
    var indices : *index Buffer<[]uint>;
    var fragColor : *ColorOutput<PreferredPixelFormat>;
    var bindings : *BindGroup<Bindings>;
};

var device = new Device();
var image = new Image<RGBA8unorm>(inline("third_party/libjpeg-turbo/testimages/testorig.jpg"));
var imageSize = image.GetSize();
var texture = new sampleable Texture2D<RGBA8unorm>(device, imageSize);
var buffer = new hostwriteable Buffer<[]ubyte<4>>(device, texture.MinBufferWidth() * imageSize.y);
image.Decode(buffer.MapWrite(), texture.MinBufferWidth());
var copyEncoder = new CommandEncoder(device);
texture.CopyFromBuffer(copyEncoder, buffer, imageSize);
device.GetQueue().Submit(copyEncoder.Finish());

var window = new Window(imageSize);
var swapChain = new SwapChain<PreferredPixelFormat>(device, window);
var verts = [4]Vertex{
  { position = {-1.0,  1.0}, texCoord = {0.0, 0.0} },
  { position = { 1.0,  1.0}, texCoord = {1.0, 0.0} },
  { position = {-1.0, -1.0}, texCoord = {0.0, 1.0} },
  { position = { 1.0, -1.0}, texCoord = {1.0, 1.0} }
};
var indices = [6]uint{ 0, 1, 2, 1, 2, 3 };
var vb = new vertex Buffer<[]Vertex>(device, &verts);
var ib = new index Buffer<[]uint>(device, &indices);
var pipeline = new RenderPipeline<Pipeline>(device);
var bindings = Bindings{
  sampler = new Sampler(device),
  textureView = texture.CreateSampleableView()
};
var bindGroup = new BindGroup<Bindings>(device, &bindings);
var encoder = new CommandEncoder(device);
var p = Pipeline{
  vertices = new VertexInput<Vertex>(vb),
  indices = ib,
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
