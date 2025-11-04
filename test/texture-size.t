include "include/test.t"
include "include/string.t"
var device = new Device();

var texture1d = new sampleable Texture1D<RGBA8unorm>(device, 42, 1);
Test.Expect(texture1d.GetSize() == 42);
Test.Expect(texture1d.GetSize(0) == 42);

var texture2d = new sampleable Texture2D<RGBA8unorm>(device, {4, 2}, 2);
Test.Expect(Math.all(texture2d.GetSize() == uint<2>{4, 2}));
Test.Expect(Math.all(texture2d.GetSize(0) == uint<2>{4, 2}));
Test.Expect(Math.all(texture2d.GetSize(1) == uint<2>{2, 1}));

var texture3d = new sampleable Texture3D<RGBA8unorm>(device, {6, 3, 5}, 2);
Test.Expect(Math.all(texture3d.GetSize() == uint<3>{6, 3, 5}));
Test.Expect(Math.all(texture3d.GetSize(0) == uint<3>{6, 3, 5}));
Test.Expect(Math.all(texture3d.GetSize(1) == uint<3>{3, 1, 5}));

var textureCube = new sampleable TextureCube<RGBA8unorm>(device, {2, 4}, 2);
Test.Expect(Math.all(textureCube.GetSize() == uint<2>{2, 4}));
Test.Expect(Math.all(textureCube.GetSize(0) == uint<2>{2, 4}));
Test.Expect(Math.all(textureCube.GetSize(1) == uint<2>{1, 2}));

var texture2DArray = new sampleable Texture2DArray<RGBA8unorm>(device, {4, 8}, 3, 2);
Test.Expect(Math.all(texture2DArray.GetSize() == uint<2>{4, 8}));
Test.Expect(Math.all(texture2DArray.GetSize(0) == uint<2>{4, 8}));
Test.Expect(Math.all(texture2DArray.GetSize(1) == uint<2>{2, 4}));
Test.Expect(Math.all(texture2DArray.GetSize(2) == uint<2>{1, 2}));
Test.Expect(texture2DArray.GetNumLayers() == 3);
