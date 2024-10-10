include "include/test.t"

var s : ushort = 65535us;
Test.Expect((uint) s == 65535u);
Test.Expect((int) s == -1);
