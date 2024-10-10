include "include/test.t"

var a = 2.0d;
var b : double;
var c : double;
b = 3.0d;
c = a * b;
Test.Expect(c == 6.0d);
