include "include/test.t"

class Uniforms {
  var array : [3]int;
}

var uniforms : Uniforms;
uniforms.array = {42, 21, 7};

var device = new Device();

var uniformBuf = new uniform Buffer<Uniforms>(device, &uniforms);
var hostBuf = new readonly Buffer<Uniforms>(device);

var encoder = new CommandEncoder(device);
hostBuf.CopyFromBuffer(encoder, uniformBuf);
device.GetQueue().Submit(encoder.Finish());

var result = hostBuf.Map();
Test.Expect(result.array[0] == 42);
Test.Expect(result.array[1] == 21);
Test.Expect(result.array[2] == 7);
hostBuf.Unmap();
