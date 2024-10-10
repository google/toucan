include "include/test.t"

var f = 1.0;
do {
  f += 1.0;
} while (f < 10.0);
Test.Expect(f == 10.0);
