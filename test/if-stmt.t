include "include/test.t"

var f : float;
var i = 3;
if (i > 1) {
  f = 3.0;
} else {
  f = 4.0;
}
Test.Expect(f == 3.0);
