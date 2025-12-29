include "include/test.t"

var a = short<2>(-3s, 32767s);
Test.Expect(a.x as int == -3);
Test.Expect(a.y as int == 32767);
