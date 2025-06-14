var device = new Device();
var buffer = new hostwriteable Buffer<int>(device);
var data1 = buffer.MapWrite();
var data2 = buffer.MapWrite();
data2: = 21;
data2 = null;
data1: = 42;
