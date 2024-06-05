include "include/test.t"

ushort s = 65535us;
Test.Expect((uint) s == 65535u);
Test.Expect((int) s == -1);
return (float) s;
