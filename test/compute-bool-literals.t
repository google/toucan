#include "include/test.t"

class Bindings {
  var result : *storage Buffer<[]int>;
}

class Pipeline {
  compute(1, 1, 1) main(cb : &ComputeBuiltins) {
    var result = bindings.Get().result.MapWrite();
    if (true) result[0] = 42;
    if (!false) result[1] = 21;
  }
  var bindings : *BindGroup<Bindings>;
}

var device = new Device();

var computePipeline = new ComputePipeline<Pipeline>(device);

var storageBuf = new storage Buffer<[]int>(device, 2);
var hostBuf = new hostreadable Buffer<[]int>(device, 2);

var bg = new BindGroup<Bindings>(device, {result = storageBuf});

var encoder = new CommandEncoder(device);
var computePass = new ComputePass<Pipeline>(encoder, {bindings = bg});
computePass.SetPipeline(computePipeline);
computePass.Dispatch(1, 1, 1);
computePass.End();
hostBuf.CopyFromBuffer(encoder, storageBuf);
device.GetQueue().Submit(encoder.Finish());

var result = hostBuf.MapRead();
Test.Expect(result[0] == 42);
Test.Expect(result[1] == 21);
