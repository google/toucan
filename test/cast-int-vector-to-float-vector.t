include "include/test.t"

var a = int<4>(-3, 101, 2, 0);
var b = a as float<4>;
Test.Expect(b.x == -3.0);
Test.Expect(b.y == 101.0);
Test.Expect(b.z == 2.0);
Test.Expect(b.w == 0.0);
