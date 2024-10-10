include "include/test.t"

var i : int;
var f : float;
var g : float;
f = 3.0;
g = 1.1;
for (i = 0; i < 100; i = i + 1) {
  f = f * g;
}
Test.Expect(f == 41341.914062);
