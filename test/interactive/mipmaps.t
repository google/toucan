class Vertex {
  var position : float<2>;
  var texCoord : float<2>;
}

class Transform3 {
  static Scale(v : float<2>) : float<3,3> {
    return float<3,3>{float<3>{v.x, 0.0, 0.0},
                      float<3>{0.0, v.y, 0.0},
                      float<3>{0.0, 0.0, 1.0}};
  }
  static Translation(v : float<2>) : float<3,3> {
    return float<3,3>{float<3>{1.0, 0.0, 0.0},
                      float<3>{0.0, 1.0, 0.0},
                      float<3>{v.x, v.y, 1.0}};
  }
}

class Uniforms {
  var matrix : float<3,3>;
}

class Bindings {
  var sampler : *Sampler;
  var texture : *SampleableTexture2D<float>;
  var uniforms : *uniform Buffer<Uniforms>;
}

class Pipeline {
    vertex main(vb : &VertexBuiltins) : float<2> {
        var u = bindings.Get().uniforms.MapRead();
        var v = vertices.Get();
        var pos = u.matrix * float<3>{@v.position, 1.0};
        vb.position = {@pos, 1.0};
        return v.texCoord;
    }
    fragment main(fb : &FragmentBuiltins, texCoord : float<2>) {
      var b = bindings.Get();
      fragColor.Set(b.texture.Sample(b.sampler, texCoord));
    }
    var vertices : *VertexInput<Vertex>;
    var indices : *index Buffer<[]uint>;
    var fragColor : *ColorOutput<PreferredPixelFormat>;
    var bindings : *BindGroup<Bindings>;
};

var device = new Device();

var window = new Window({1023 + 11, 512});
var swapChain = new SwapChain<PreferredPixelFormat>(device, window);

var verts = [4]Vertex{
  { position = { 0.0,  1.0}, texCoord = {0.0, 0.0} },
  { position = { 1.0,  1.0}, texCoord = {1.0, 0.0} },
  { position = { 0.0,  0.0}, texCoord = {0.0, 1.0} },
  { position = { 1.0,  0.0}, texCoord = {1.0, 1.0} }
};

var indices = [6]uint{ 0, 1, 2, 1, 2, 3 };

var pipeline = new RenderPipeline<Pipeline>(device);
var encoder = new CommandEncoder(device);
var texSize = uint<2>{512, 512};
var mipCount = 10;
var texture = new sampleable Texture2D<RGBA8unorm>(device, texSize, mipCount);
var mipColors = [10]ubyte<4>{
  { 255ub,   0ub,   0ub, 255ub },
  { 255ub, 255ub,   0ub, 255ub },
  {   0ub, 255ub,   0ub, 255ub },
  {   0ub, 255ub, 255ub, 255ub },
  {   0ub,   0ub, 255ub, 255ub },
  { 255ub,   0ub, 255ub, 255ub },
  { 255ub, 255ub, 255ub, 255ub },
  { 255ub, 128ub,   0ub, 255ub },
  { 128ub, 255ub,   0ub, 255ub },
  {   0ub, 255ub, 128ub, 255ub }
};
var width = texture.MinBufferWidth();
var mipSize = uint<2>{ width, texSize.y };
for (var mipLevel = 0; mipLevel < mipCount; ++mipLevel) {
  var buffer = new hostwriteable Buffer<[]ubyte<4>>(device, width * mipSize.y);
  var data = buffer.MapWrite();
  for (var y = 0u; y < mipSize.y; ++y) {
    for (var x = 0u; x < mipSize.x; ++x) {
      data[y * width + x] = mipColors[mipLevel];
    }
  }
  data = null;
  texture.CopyFromBuffer(encoder, buffer, mipSize, {0, 0}, mipLevel);
  mipSize.x /= 2;
  mipSize.y /= 2;
}
device.GetQueue().Submit(encoder.Finish());

var bindings : Bindings;

var vb = new vertex Buffer<[]Vertex>(device, &verts);

var viewMatrix = Transform3.Translation({-1.0, -1.0});
viewMatrix *= Transform3.Scale(2.0 / window.GetSize() as float<2>);

var indexBuffer = new index Buffer<[]uint>(device, &indices);
bindings.sampler = new Sampler(device);

while (System.IsRunning()) {
  var encoder = new CommandEncoder(device);
  var fb = swapChain.GetCurrentTexture().CreateColorOutput(LoadOp.Load);

  var position = int<2>{0, 0};
  var size = texSize;
  for (var mipLevel = 0; mipLevel < mipCount; ++mipLevel) {
    bindings.texture = texture.CreateSampleableView(baseMipLevel = mipLevel, mipLevelCount = 1);
    var modelMatrix = Transform3.Translation(position as float<2>);
    modelMatrix *= Transform3.Scale(size as float<2>);
    bindings.uniforms = new uniform Buffer<Uniforms>(device, { matrix = viewMatrix * modelMatrix });

    var renderPass = new RenderPass<Pipeline>(encoder, {
      fragColor = fb,
      vertices = new VertexInput<Vertex>(vb),
      indices = indexBuffer,
      bindings = new BindGroup<Bindings>(device, &bindings)
    });
    renderPass.SetPipeline(pipeline);
    renderPass.DrawIndexed(indices.length, 1, 0, 0, 0);
    renderPass.End();

    position.x += size.x + 1;
    size /= int<2>{2};
  }

  var cb = encoder.Finish();
  device.GetQueue().Submit(cb);
  swapChain.Present();

  while (System.HasPendingEvents()) {
    System.GetNextEvent();
  }
}
