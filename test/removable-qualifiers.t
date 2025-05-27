// Test removable qualifiers
var b : *Buffer<float>;
var u : *uniform Buffer<float>;
var s : *storage Buffer<float>;
var v : *vertex Buffer<float>;
var i : *index Buffer<float>;
var us : *uniform storage Buffer<float>;
var hr : *hostreadable Buffer<float>;
var hw : *hostwriteable Buffer<float>;
b = u;
b = s;
b = v;
b = i;
b = us;
u = us;
s = us;
b = hr;
b = hw;

var t : *Texture2D<RGBA8unorm>;
var st : *sampleable Texture2D<RGBA8unorm>;
var rt : *renderable Texture2D<RGBA8unorm>;
var srt : *sampleable renderable Texture2D<RGBA8unorm>;

t = st;
t = rt;
t = srt;
st = srt;
rt = srt;
