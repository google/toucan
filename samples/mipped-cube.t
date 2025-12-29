include "cube.t"
include "event-handler.t"
include "quaternion.t"
include "transform.t"
include "mipmap-generator.t"

var device = new Device();

var image = new Image<RGBA8unorm>(inline("third_party/libjpeg-turbo/testimages/testorig.jpg"));
var imageSize = image.GetSize();

var mipCount = 30 - Math.clz(Math.max(imageSize.x, imageSize.y));
var texture = new renderable sampleable Texture2D<RGBA8unorm>(device, imageSize, mipCount);
var buffer = new hostwriteable Buffer<[]ubyte<4>>(device, texture.MinBufferWidth() * imageSize.y);
image.Decode(buffer.MapWrite(), texture.MinBufferWidth());
var copyEncoder = new CommandEncoder(device);
texture.CopyFromBuffer(copyEncoder, buffer, imageSize);
device.GetQueue().Submit(copyEncoder.Finish());

MipmapGenerator<RGBA8unorm>.Generate(device, texture);
var window = new Window(System.GetScreenSize());
var swapChain = new SwapChain<PreferredPixelFormat>(device, window);

class Uniforms {
  var model       : float<4,4>;
  var view        : float<4,4>;
  var projection  : float<4,4>;
}

class Bindings {
  var sampler : *Sampler;
  var textureView : *SampleableTexture2D<float>;
  var uniforms : *uniform Buffer<Uniforms>;
}

class DrawPipeline {
    vertex main(vb : &VertexBuiltins) : float<2> {
        var v = vertices.Get();
        var uniforms = bindings.Get().uniforms.MapRead();
        var pos = float<4>(@v, 1.0);
        vb.position = uniforms.projection * uniforms.view * uniforms.model * pos;
        return texCoords.Get();
    }

    fragment main(fb : &FragmentBuiltins, texCoord : float<2>) {
      var b = bindings.Get();
      fragColor.Set(b.textureView.Sample(b.sampler, texCoord));
    }

    var vertices : *VertexInput<float<3>>;
    var texCoords : *VertexInput<float<2>>;
    var indexBuffer : *index Buffer<[]uint>;
    var fragColor : *ColorOutput<PreferredPixelFormat>;
    var depth : *DepthStencilOutput<Depth24Plus>;
    var bindings : *BindGroup<Bindings>;
};

var pipeline = new RenderPipeline<DrawPipeline>(device);
var bindings = Bindings{
  sampler = new Sampler(device),
  textureView = texture.CreateSampleableView(),
  uniforms = new uniform Buffer<Uniforms>(device)
};

var vb = new vertex Buffer<[]float<3>>(device, &cubeVerts);
var tcb = new vertex Buffer<[]float<2>>(device, &cubeTexCoords);
var pipelineData = DrawPipeline{
  vertices = new VertexInput<float<3>>(vb),
  texCoords = new VertexInput<float<2>>(tcb),
  indexBuffer = new index Buffer<[]uint>(device, &cubeIndices),
  bindings = new BindGroup<Bindings>(device, &bindings)
};

var handler = EventHandler{ distance = 3.0 };
var teapotQuat = Quaternion(float<3>(1.0, 0.0, 0.0), -Math.pi / 2.0);
teapotQuat.normalize();
var teapotRotation = teapotQuat.toMatrix();
var depthBuffer = new renderable Texture2D<Depth24Plus>(device, window.GetSize());
var uniforms : Uniforms;
var prevWindowSize = uint<2>{0, 0};
while (System.IsRunning()) {
  var orientation = Quaternion(float<3>(0.0, 1.0, 0.0), handler.rotation.x);
  orientation = orientation.mul(Quaternion(float<3>(1.0, 0.0, 0.0), handler.rotation.y));
  orientation.normalize();
  var newSize = window.GetSize();
  if (Math.any(newSize != prevWindowSize)) {
    swapChain.Resize(newSize);
    depthBuffer = new renderable Texture2D<Depth24Plus>(device, newSize);
    var aspectRatio = newSize.x as float / newSize.y as float;
    uniforms.projection = Transform.projection(0.5, 200.0, -aspectRatio, aspectRatio, -1.0, 1.0);
    prevWindowSize = newSize;
  }
  uniforms.view = Transform.translation({0.0, 0.0, -handler.distance});
  uniforms.view *= orientation.toMatrix();
  uniforms.model = Transform.scale({1.0, 1.0, 1.0});
  bindings.uniforms.SetData(&uniforms);
  uniforms.model = teapotRotation * Transform.scale({2.0, 2.0, 2.0});
  var encoder = new CommandEncoder(device);
  var fb = swapChain.GetCurrentTexture().CreateColorOutput(LoadOp.Clear);
  var db = depthBuffer.CreateDepthStencilOutput(LoadOp.Clear);
  var renderPass = new RenderPass<DrawPipeline>(encoder, { fragColor = fb, depth = db });

  var cubePass = new RenderPass<DrawPipeline>(renderPass);
  cubePass.SetPipeline(pipeline);
  cubePass.Set(&pipelineData);
  cubePass.DrawIndexed(cubeIndices.length, 1, 0, 0, 0);

  renderPass.End();
  var cb = encoder.Finish();
  device.GetQueue().Submit(cb);
  swapChain.Present();

  do {
    handler.Handle(System.GetNextEvent());
  } while (System.HasPendingEvents());
}
