{
  var b : *Buffer<float>;
  var u : *uniform Buffer<float>;
  var s : *storage Buffer<float>;
  var us : *uniform storage Buffer<float>;
  var hr : *hostreadable Buffer<float>;
  var hw : *hostwriteable Buffer<float>;
  b = u;
  b = s;
  b = us;
  u = us;
  s = us;
  b = hr;
  b = hw;
}

{
  var b : *Buffer<[]uint>;
  var s : *storage Buffer<[]uint>;
  var v : *vertex Buffer<[]uint>;
  var i : *index Buffer<[]uint>;
  var vs : *vertex storage Buffer<[]uint>;
  var is : *index storage Buffer<[]uint>;
  var vis : *vertex index storage Buffer<[]uint>;
  b = s;
  b = v;
  b = i;
  s = vs;
  s = is;
  s = vis;
  i = is;
  i = vis;
  v = vs;
  v = vis;
  vs = vis;
  is = vis;
}

var t : *Texture2D<RGBA8unorm>;
var st : *sampleable Texture2D<RGBA8unorm>;
var rt : *renderable Texture2D<RGBA8unorm>;
var srt : *sampleable renderable Texture2D<RGBA8unorm>;

t = st;
t = rt;
t = srt;
st = srt;
rt = srt;
