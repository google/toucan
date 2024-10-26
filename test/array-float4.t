include "include/test.t"

var count = 200;
var a : [256]float<4>;
var mul = float<4>(1.00001, 1.00001, 1.00001, 1.00001);
var add = float<4>(1.0, 1.0, 1.0, 1.0);
for (var i = 0; i < a.length; ++i) {
 a[i] = float<4>(2000000.0, 2000000.0, 2000000.0, 2000000.0);
}
for (var j = 0; j < count; ++j) {
  for (var i = 0; i < a.length; ++i) {
    a[i] = a[i] * mul + add;
  }
}
Test.Expect(a[1].x == 2004203.875);
