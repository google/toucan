include "include/test.t"

float<3> v31 = float<3>(1.0);
float<3> v32 = float<3>(2.0, 3.0);
float<3> v33 = float<3>(4.0, 5.0, 6.0);

Test.Expect(v31.x == 1.0);
Test.Expect(v32.x == 2.0);
Test.Expect(v32.y == 3.0);
Test.Expect(v33.x == 4.0);
Test.Expect(v33.y == 5.0);
Test.Expect(v33.z == 6.0);
