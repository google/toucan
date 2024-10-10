include "include/test.t"

var a = short<2>(-3s, 32767s);
Test.Expect((int) a.x == -3);
Test.Expect((int) a.y == 32767);
