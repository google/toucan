include "include/test.t"

var a = int<2>(0, 1);
var b = int<2>(2, 1);
var c = int<2>(3, 4);
Test.Expect(Math.all(a == a));
Test.Expect(!Math.all(a == b));
Test.Expect(Math.any(a == b));
Test.Expect(!Math.any(a == c));
Test.Expect(!Math.any(a != a));
Test.Expect(!Math.all(a != b));
