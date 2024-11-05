include "include/test.t"

class ComputeBindings {
  var buffer : *writeonly storage Buffer<[]int>;
}

class Struct {
  var a : int;
}

class Compute {
  static set(p : ^int, value : int) {
    p: = value;
  }
  compute(1, 1,1) main(cb : ^ComputeBuiltins) {
    var buffer = bindings.Get().buffer.Map();
    var i : int;
    var array : [1]int;
    var struct : Struct;
    this.set(&i, 7);
    this.set(&array[0], 21);
    this.set(&struct.a, 42);
    buffer[0] = i;
    buffer[1] = array[0];
    buffer[2] = struct.a;
  }
  var bindings : *BindGroup<ComputeBindings>;
}

var device = new Device();

var computePipeline = new ComputePipeline<Compute>(device);

var storageBuf = new writeonly storage Buffer<[]int>(device, 3);
var hostBuf = new readonly Buffer<[]int>(device, 3);

var bg = new BindGroup<ComputeBindings>(device, {buffer = storageBuf});

var encoder = new CommandEncoder(device);
var computePass = new ComputePass<Compute>(encoder, null);
computePass.SetPipeline(computePipeline);
computePass.Set({bindings = bg});
computePass.Dispatch(1, 1, 1);
computePass.End();
hostBuf.CopyFromBuffer(encoder, storageBuf);
device.GetQueue().Submit(encoder.Finish());

Test.Expect(hostBuf.Map()[0] == 7);
Test.Expect(hostBuf.Map()[1] == 21);
Test.Expect(hostBuf.Map()[2] == 42);
