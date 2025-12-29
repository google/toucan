include "include/test.t"

var image = new Image<RGBA8unorm>(null);
Test.Expect(image == null);

var data = inline("test/include/small.jpg");

image = new Image<RGBA8unorm>(data);
var d = [1] new writeonly ubyte<4>;
image.Decode(d, 1);
Test.Expect(d[0].r as uint == 190u);
Test.Expect(d[0].g as uint == 190u);
Test.Expect(d[0].b as uint == 190u);
Test.Expect(d[0].a as uint == 255u);

var copy = [data.length] new ubyte;
for (var i = 0; i < data.length; ++i) {
  copy[i] = data[i];
}

image = new Image<RGBA8unorm>(copy);
copy = null;
d[0] = ubyte<4>{};
image.Decode(d, 1);
Test.Expect(d[0].r as uint == 190u);
Test.Expect(d[0].g as uint == 190u);
Test.Expect(d[0].b as uint == 190u);
Test.Expect(d[0].a as uint == 255u);
