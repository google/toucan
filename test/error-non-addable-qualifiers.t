var b : *Buffer<[]uint>;
var fb : *Buffer<float>;
var u : *uniform Buffer<float> = fb;
var s : *storage Buffer<[]uint> = b;
var v : *vertex Buffer<[]uint> = b;
var i : *index Buffer<[]uint> = b;
var us : *uniform storage Buffer<float> = fb;
var hr : *hostreadable Buffer<[]uint> = b;
var hw : *hostwriteable Buffer<[]uint> = b;

var t : *Texture2D<RGBA8unorm>;
var st : *sampleable Texture2D<RGBA8unorm> = t;
var rt : *renderable Texture2D<RGBA8unorm> = t;
var srt : *sampleable renderable Texture2D<RGBA8unorm> = t;
