include "include/test.t"

var a = ushort<2>(5us, 65535us);
Test.Expect(a.x as uint == 5);
Test.Expect(a.y as uint == 65535);
Test.Expect(a.y as int == -1);
