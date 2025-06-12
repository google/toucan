include "include/test.t"

class ComputeBindings {
  var buffer : *storage Buffer<[]int>;
}

class Compute {
  compute(1, 1, 1) main(cb : &ComputeBuiltins) {
    var buffer = bindings.Get().buffer.MapWrite();
    var a = int<2>(0, 1);
    var b = int<2>(2, 1);
    var c = int<2>(3, 4);
    if (Math.all(a == a)) buffer[0] = 1;
    if (Math.all(a == b)) buffer[1] = 1;
    if (Math.any(a == b)) buffer[2] = 1;
    if (Math.any(a == c)) buffer[3] = 1;
    if (Math.any(a != a)) buffer[4] = 1;
    if (Math.all(a != b)) buffer[5] = 1;
  }
  var bindings : *BindGroup<ComputeBindings>;
}

var device = new Device();

var computePipeline = new ComputePipeline<Compute>(device);

var storageBuf = new storage Buffer<[]int>(device, 6);
var hostBuf = new hostreadable Buffer<[]int>(device, 6);

var bg = new BindGroup<ComputeBindings>(device, {buffer = storageBuf});

var encoder = new CommandEncoder(device);
var computePass = new ComputePass<Compute>(encoder, {bindings = bg});
computePass.SetPipeline(computePipeline);
computePass.Dispatch(1, 1, 1);
computePass.End();
hostBuf.CopyFromBuffer(encoder, storageBuf);
device.GetQueue().Submit(encoder.Finish());

var result = hostBuf.MapRead();
Test.Expect(result[0] == 1);
Test.Expect(result[1] == 0);
Test.Expect(result[2] == 1);
Test.Expect(result[3] == 0);
Test.Expect(result[4] == 0);
Test.Expect(result[5] == 0);
