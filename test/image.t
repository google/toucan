include "include/test.t"

var image = new Image<RGBA8unorm>(null);
Test.Expect(image == null);

var data = inline("test/include/small.jpg");

image = new Image<RGBA8unorm>(data);
var d = [1] new writeonly ubyte<4>;
image.Decode(d, 1);
Test.Expect((uint) d[0].r == 190u);
Test.Expect((uint) d[0].g == 190u);
Test.Expect((uint) d[0].b == 190u);
Test.Expect((uint) d[0].a == 255u);

var copy = [data.length] new ubyte;
for (var i = 0; i < data.length; ++i) {
  copy[i] = data[i];
}

image = new Image<RGBA8unorm>(copy);
copy = null;
d[0] = ubyte<4>{};
image.Decode(d, 1);
Test.Expect((uint) d[0].r == 190u);
Test.Expect((uint) d[0].g == 190u);
Test.Expect((uint) d[0].b == 190u);
Test.Expect((uint) d[0].a == 255u);
