include "include/test.t"

var a = byte<2>(-3b, -5b);
Test.Expect((int) a.x == -3);
Test.Expect((int) a.y == -5);
