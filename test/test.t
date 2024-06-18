include "include/test.t"

float<4> v = float<4>(5.0, 0.0, 2.0, 3.0) / float<4>(2.0, 2.0, 2.0, 2.0);
Test.Expect(v.x == 2.5);
Test.Expect(v.y == 0.0);
Test.Expect(v.z == 1.0);
Test.Expect(v.w == 1.5);
