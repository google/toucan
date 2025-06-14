var device = new Device();
var buffer = new hostwriteable Buffer<int>(device);
var data = buffer.MapWrite();
buffer = null;
data: = 42;
