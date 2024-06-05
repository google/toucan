include "include/test.t"

ubyte<2> a = ubyte<2>(5ub, 3ub);
Test.Expect((int) a.x == 5);
Test.Expect((int) a.y == 3);
return (float) a.y;
