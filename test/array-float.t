include "include/test.t"

var outer_count = 200;
var a : [1024]float;
for (var i = 0; i < a.length; i++) {
  a[i] = 2000000.0;
}
for (var j = 0; j < outer_count; ++j) {
  for (var i = 0; i < a.length; ++i) {
    a[i] = a[i] * 1.00001 + 1.0;
  }
}

Test.Expect(a[0] == 2004203.875);
