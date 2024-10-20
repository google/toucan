var device = new Device();
var tex = new Texture2D<RGBA8unorm>(device, {2, 2});
var view = tex.CreateSampleableView();
