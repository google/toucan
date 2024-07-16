include "include/test.t"

class ComputeBindings {
  writeonly storage Buffer<int[]>* buffer;
}

class Struct {
  int a;
}

class Compute {
  static void set(int^ p, int value) {
    *p = value;
  }
  void computeShader(ComputeBuiltins cb) compute(1, 1, 1) {
    auto buffer = bindings.Get().buffer.MapReadWriteStorage();
    int i;
    int[1] array;
    Struct struct;
    this.set(&i, 7);
    this.set(&array[0], 21);
    this.set(&struct.a, 42);
    buffer[0] = i;
    buffer[1] = array[0];
    buffer[2] = struct.a;
  }
  BindGroup<ComputeBindings>* bindings;
}

Device* device = new Device();

ComputePipeline* computePipeline = new ComputePipeline<Compute>(device);

auto storageBuf = new writeonly storage Buffer<int[]>(device, 3);
auto hostBuf = new readonly Buffer<int[]>(device, 3);

auto bg = new BindGroup<ComputeBindings>(device, {buffer = storageBuf});

auto encoder = new CommandEncoder(device);
auto computePass = new ComputePass<Compute>(encoder, null);
computePass.SetPipeline(computePipeline);
computePass.Set({bindings = bg});
computePass.Dispatch(1, 1, 1);
computePass.End();
encoder.CopyBufferToBuffer(storageBuf, hostBuf);
device.GetQueue().Submit(encoder.Finish());

Test.Expect(hostBuf.MapRead()[0] == 7);
Test.Expect(hostBuf.MapRead()[1] == 21);
Test.Expect(hostBuf.MapRead()[2] == 42);
