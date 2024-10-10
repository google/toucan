include "include/test.t"

var a = 3.0;
var b = 2.0;
var c = 0.0;
var d = 1.0;

Test.Expect(a++ == 3.0);
Test.Expect(--b == 1.0);
Test.Expect(++c == 1.0);
Test.Expect(d-- == 1.0);
