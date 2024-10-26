include "include/test.t"

class ComputeBindings {
  var buffer : *writeonly storage Buffer<[]int>;
}

class Compute {
  compute(1, 1, 1) main(cb : ^ComputeBuiltins) {
    var buffer = bindings.Get().buffer.Map();
    buffer[0] = 42;
  }
  var bindings : *BindGroup<ComputeBindings>;
}

var device = new Device();

var computePipeline = new ComputePipeline<Compute>(device);

var storageBuf = new writeonly storage Buffer<[]int>(device, 1);
var hostBuf = new readonly Buffer<[]int>(device, 1);

var bg = new BindGroup<ComputeBindings>(device, {buffer = storageBuf});

var encoder = new CommandEncoder(device);
var computePass = new ComputePass<Compute>(encoder, null);
computePass.SetPipeline(computePipeline);
computePass.Set({bindings = bg});
computePass.Dispatch(1, 1, 1);
computePass.End();
hostBuf.CopyFromBuffer(encoder, storageBuf);
device.GetQueue().Submit(encoder.Finish());

Test.Expect(hostBuf.Map()[0] == 42);
