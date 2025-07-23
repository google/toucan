var device = new Device();
class C {
  var i : int;
  var sp : *int;
  var wp : ^int;
  var a : [3]float;
  var v : int<3>;
  var s : *storage Sampler;
  var t : Texture2D<RGBA8unorm>;
  var vb : *vertex Buffer<float<4>>;
  var ib : *index Buffer<float<4>>;
  var ib : *hostreadable Buffer<float<4>>;
  var ib : *hostwriteable Buffer<float<4>>;
  var ra : []byte;
}
new BindGroup<float>(device, null);
new BindGroup<C>(device, {});
