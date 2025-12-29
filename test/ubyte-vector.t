include "include/test.t"

var a = ubyte<2>(5ub, 3ub);
Test.Expect(a.x as int == 5);
Test.Expect(a.y as int == 3);
