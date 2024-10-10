include "include/test.t"

var a = 3s;
var b = 2s;
var c = 0s;
var d = 1s;
Test.Expect((int) a++ == 3);
Test.Expect((int) --b == 1);
Test.Expect((int) ++c == 1);
Test.Expect((int) d-- == 1);
