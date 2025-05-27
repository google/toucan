// Test non-addable qualifiers
var b : *Buffer<float>;
var u : *uniform Buffer<float> = b;
var s : *storage Buffer<float> = b;
var v : *vertex Buffer<float> = b;
var i : *index Buffer<float> = b;
var us : *uniform storage Buffer<float> = b;
var hr : *hostreadable Buffer<float> = b;
var hw : *hostwriteable Buffer<float> = b;

var t : *Texture2D<RGBA8unorm>;
var st : *sampleable Texture2D<RGBA8unorm> = t;
var rt : *renderable Texture2D<RGBA8unorm> = t;
var srt : *sampleable renderable Texture2D<RGBA8unorm> = t;
