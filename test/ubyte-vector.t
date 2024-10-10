include "include/test.t"

var a = ubyte<2>(5ub, 3ub);
Test.Expect((int) a.x == 5);
Test.Expect((int) a.y == 3);
