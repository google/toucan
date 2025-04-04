var device = new Device();
var window = new Window(device, {0, 0}, {640, 480});
var tex = new sampleable Texture1D<RGBA8unorm>(device, 1);
var buffer = new writeonly Buffer<[]float<4>>(device);
var data = buffer.Map();
data[0] = float<4>(1.0, 1.0, 1.0, 1.0);
buffer.Unmap();
var encoder = new CommandEncoder(device);
tex.CopyFromBuffer(encoder, buffer, 1);
device.GetQueue().Submit(encoder.Finish());
