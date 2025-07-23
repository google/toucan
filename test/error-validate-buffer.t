var device = new Device();
new vertex Buffer<int>(device);
new vertex Buffer<[]int>(device); // should succeed
new vertex Buffer<[]byte>(device);
new vertex Buffer<[]ubyte>(device);
new vertex Buffer<[]short>(device);
new vertex Buffer<[]short<2>>(device);
new vertex Buffer<[]ushort>(device);
new vertex Buffer<[]ushort<2>>(device);
new vertex Buffer<[]double>(device);
new vertex Buffer<[]double<2>>(device);

class C {
  var b : byte;
  var ub : ubyte;
  var s : short;
  var us : ushort;
  var d : double;
}

new vertex Buffer<[]C>(device);

new index Buffer<[]float>(device);
new index Buffer<uint>(device);
new index Buffer<[3]float>(device);

new uniform Buffer<byte>(device);
new uniform Buffer<ushort<4>>(device);
new uniform Buffer<[3]double>(device);
new uniform Buffer<[]float>(device);
new uniform Buffer<*float>(device);
new uniform Buffer<^float>(device);
new uniform Buffer<&float>(device);
new uniform Buffer<C>(device);

new storage Buffer<byte>(device);
new storage Buffer<ushort<4>>(device);
new storage Buffer<[3]double>(device);
new storage Buffer<*float>(device);
new storage Buffer<^float>(device);
new storage Buffer<&float>(device);
new storage Buffer<C>(device);

new hostreadable vertex Buffer<[]uint>(device);
new hostreadable index Buffer<[]uint>(device);
new hostreadable storage Buffer<float>(device);
new hostreadable uniform Buffer<float>(device);

new hostwriteable vertex Buffer<[]uint>(device);
new hostwriteable index Buffer<[]uint>(device);
new hostwriteable storage Buffer<float>(device);
new hostwriteable uniform Buffer<float>(device);

new sampleable renderable readonly writeonly unfilterable Buffer<float>(device);
