include "include/test.t"

class ComputeBindings {
  writeonly storage Buffer<int[]>* buffer;
}

class Compute {
  void computeShader(ComputeBuiltins^ cb) compute(1, 1, 1) {
    var buffer = bindings.Get().buffer.MapWriteStorage();
    buffer[0] = 42;
  }
  BindGroup<ComputeBindings>* bindings;
}

Device* device = new Device();

ComputePipeline* computePipeline = new ComputePipeline<Compute>(device);

var storageBuf = new writeonly storage Buffer<int[]>(device, 1);
var hostBuf = new readonly Buffer<int[]>(device, 1);

var bg = new BindGroup<ComputeBindings>(device, {buffer = storageBuf});

var encoder = new CommandEncoder(device);
var computePass = new ComputePass<Compute>(encoder, null);
computePass.SetPipeline(computePipeline);
computePass.Set({bindings = bg});
computePass.Dispatch(1, 1, 1);
computePass.End();
encoder.CopyBufferToBuffer(storageBuf, hostBuf);
device.GetQueue().Submit(encoder.Finish());

Test.Expect(hostBuf.MapRead()[0] == 42);
