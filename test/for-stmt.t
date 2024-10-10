include "include/test.t"

var a = 1.0001;
var b = 2.7;
var c = 2.7;
for(var i = 0; i < 1000; i = i + 1) {
  c = a * b;
}
Test.Expect(b == 2.7);
