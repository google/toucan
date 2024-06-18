include "include/test.t"

ubyte[]^ t = inline("test/foo.txt");
Test.Expect((int) t[0] == 97);
