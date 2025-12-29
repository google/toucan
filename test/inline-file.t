include "include/test.t"

var t : ^[]ubyte = inline("test/foo.txt");
Test.Expect(t[0] as int == 97);
