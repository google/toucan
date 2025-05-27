include "include/test.t"

class ComputeBindings {
  var buffer : *writeonly storage Buffer<int<4>>;
}

class Compute {
  compute(1, 1, 1) main(cb : &ComputeBuiltins) {
    var buffer = bindings.Get().buffer.Map();
    buffer.y = 42;
  }
  var bindings : *BindGroup<ComputeBindings>;
}

var device = new Device();

var computePipeline = new ComputePipeline<Compute>(device);

var storageBuf = new writeonly storage Buffer<int<4>>(device, 1);
var hostBuf = new hostreadable Buffer<int<4>>(device, 1);

var bg = new BindGroup<ComputeBindings>(device, {buffer = storageBuf});

var encoder = new CommandEncoder(device);
var computePass = new ComputePass<Compute>(encoder, {bindings = bg});
computePass.SetPipeline(computePipeline);
computePass.Dispatch(1, 1, 1);
computePass.End();
hostBuf.CopyFromBuffer(encoder, storageBuf);
device.GetQueue().Submit(encoder.Finish());

Test.Expect(hostBuf.Map().y == 42);
