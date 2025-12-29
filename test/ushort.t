include "include/test.t"

var s : ushort = 65535us;
Test.Expect(s as uint == 65535u);
Test.Expect(s as int == -1);
