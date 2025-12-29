include "include/test.t"

var a = byte<2>(-3b, -5b);
Test.Expect(a.x as int == -3);
Test.Expect(a.y as int == -5);
