include "include/test.t"

var t : ^[]ubyte = inline("test/foo.txt");
Test.Expect((int) t[0] == 97);
