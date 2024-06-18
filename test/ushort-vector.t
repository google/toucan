include "include/test.t"

ushort<2> a = ushort<2>(5us, 65535us);
Test.Expect((uint) a.x == 5);
Test.Expect((uint) a.y == 65535);
Test.Expect((int) a.y == -1);
