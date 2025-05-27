include "include/test.t"

class ComputeBindings {
  var buffer : *writeonly storage Buffer<int<4>>;
}

class Compute {
  compute(1, 1, 1) main(cb : &ComputeBuiltins) {
    var buffer = bindings.Get().buffer.Map();
    buffer: = buffer.wzyx;
  }
  var bindings : *BindGroup<ComputeBindings>;
}

var device = new Device();

var computePipeline = new ComputePipeline<Compute>(device);

var storageBuf = new writeonly storage Buffer<int<4>>(device, {1, 2, 3, 4});
var hostBuf = new hostreadable Buffer<int<4>>(device, 1);

var bg = new BindGroup<ComputeBindings>(device, {buffer = storageBuf});

var encoder = new CommandEncoder(device);
var computePass = new ComputePass<Compute>(encoder, {bindings = bg});
computePass.SetPipeline(computePipeline);
computePass.Dispatch(1, 1, 1);
computePass.End();
hostBuf.CopyFromBuffer(encoder, storageBuf);
device.GetQueue().Submit(encoder.Finish());

var result = hostBuf.Map();
Test.Expect(result.x == 4);
Test.Expect(result.y == 3);
Test.Expect(result.z == 2);
Test.Expect(result.w == 1);
