include "include/test.t"

var a = 3b;
var b = 2b;
var c = 0b;
var d = 1b;
Test.Expect((int) a++ == 3);
Test.Expect((int) --b == 1);
Test.Expect((int) ++c == 1);
Test.Expect((int) d-- == 1);
