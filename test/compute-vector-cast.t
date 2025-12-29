include "include/test.t"

class ComputeBindings {
  var buffer : *storage Buffer<[]uint<2>>;
}

class Compute {
  compute(1, 1, 1) main(cb : &ComputeBuiltins) {
    var buffer = bindings.Get().buffer.MapWrite();
    var f = float<2>(42.0, 21.0);
    buffer[0] = f as uint<2>;
  }
  var bindings : *BindGroup<ComputeBindings>;
}

var device = new Device();

var computePipeline = new ComputePipeline<Compute>(device);

var storageBuf = new storage Buffer<[]uint<2>>(device, 1);
var hostBuf = new hostreadable Buffer<[]uint<2>>(device, 1);

var bg = new BindGroup<ComputeBindings>(device, {buffer = storageBuf});

var encoder = new CommandEncoder(device);
var computePass = new ComputePass<Compute>(encoder, {bindings = bg});
computePass.SetPipeline(computePipeline);
computePass.Dispatch(1, 1, 1);
computePass.End();
hostBuf.CopyFromBuffer(encoder, storageBuf);
device.GetQueue().Submit(encoder.Finish());

var result = hostBuf.MapRead()[0];
Test.Expect(result.x == 42 && result.y == 21);
