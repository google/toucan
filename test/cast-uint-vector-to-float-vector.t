include "include/test.t"

uint<4> a = uint<4>(3u, 3000000000u, 2u, 0u);
float<4> b = (float<4>) a;
Test.Expect(b.x == 3.0);
Test.Expect(b.y == 3000000000.0);
Test.Expect(b.z == 2.0);
Test.Expect(b.w == 0.0);
