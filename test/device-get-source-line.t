#include "include/test.t"

class Bindings {
  var result : *storage Buffer<uint>;
}

class Pipeline {
  compute(1, 1, 1) main(cb : &ComputeBuiltins) {
    var result = bindings.Get().result.MapWrite();
    result: = System.GetSourceLine();
  }
  var bindings : *BindGroup<Bindings>;
}

var device = new Device();

var pipeline = new ComputePipeline<Pipeline>(device);

var storageBuf = new storage Buffer<uint>(device, 1);
var hostBuf = new hostreadable Buffer<uint>(device, 1);

var bindGroup = new BindGroup<Bindings>(device, {result = storageBuf});

var encoder = new CommandEncoder(device);
var computePass = new ComputePass<Pipeline>(encoder, {bindings = bindGroup});
computePass.SetPipeline(pipeline);
computePass.Dispatch(1, 1, 1);
computePass.End();
hostBuf.CopyFromBuffer(encoder, storageBuf);
device.GetQueue().Submit(encoder.Finish());

Test.Expect(hostBuf.MapRead(): == 10);
