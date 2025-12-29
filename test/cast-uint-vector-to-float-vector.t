include "include/test.t"

var a = uint<4>(3u, 3000000000u, 2u, 0u);
var b = a as float<4>;
Test.Expect(b.x == 3.0);
Test.Expect(b.y == 3000000000.0);
Test.Expect(b.z == 2.0);
Test.Expect(b.w == 0.0);
