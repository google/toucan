include "include/test.t"

class CopyBindings {
  var depth : *unfilterable SampleableTexture2D<float>;
  var result : *storage Buffer<float<4>>;
}

class DrawPass {
  var depth : *DepthStencilOutput<Depth24Plus>;
}

class CopyPipeline {
  compute(1, 1, 1) main(vb : &ComputeBuiltins) {
    var depth = bindings.Get().depth;
    var result = bindings.Get().result.Map();
    result: = depth.Load(uint<2>{0u, 0u}, 0u);
  }
  var bindings : *BindGroup<CopyBindings>;
}
var device = new Device();
var copyPipeline = new ComputePipeline<CopyPipeline>(device);
var depth = new renderable sampleable Texture2D<Depth24Plus>(device, uint<2>(1, 1));
var resultBuf = new storage Buffer<float<4>>(device, 1);
var readbackBuf = new hostreadable Buffer<float<4>>(device, 1);
var bindings = new BindGroup<CopyBindings>(device, {
  depth = depth.CreateSampleableView(),
  result = resultBuf
});
var encoder = new CommandEncoder(device);
var renderPass = new RenderPass<DrawPass>(encoder, {
  depth = depth.CreateDepthStencilOutput(depthLoadOp = LoadOp.Clear, depthClearValue = 1.0)
});
renderPass.End();
var copyPass = new ComputePass<CopyPipeline>(encoder, { bindings = bindings });
copyPass.SetPipeline(copyPipeline);
copyPass.Dispatch(1, 1, 1);
copyPass.End();
readbackBuf.CopyFromBuffer(encoder, resultBuf);
device.GetQueue().Submit(encoder.Finish());
var result = readbackBuf.MapRead()[0];
Test.Expect(result == 1.0);
