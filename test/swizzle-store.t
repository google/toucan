include "include/test.t"

var v : int<4>;
v.x = 4;
v.y = 3;
v.z = 2;
v.w = 1;
Test.Expect(v.x == 4);
Test.Expect(v.y == 3);
Test.Expect(v.z == 2);
Test.Expect(v.w == 1);
v.xz = int<2>{1, 3};
v.wy = int<2>{4, 2};
Test.Expect(v.x == 1);
Test.Expect(v.y == 2);
Test.Expect(v.z == 3);
Test.Expect(v.w == 4);
v.wzxy = int<4>{8, 7, 5, 6};
Test.Expect(v.x == 5);
Test.Expect(v.y == 6);
Test.Expect(v.z == 7);
Test.Expect(v.w == 8);

