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

return v31.x + v31.y + v31.z
     + v32.x + v32.y + v32.z
     + v33.x + v33.y + v33.z;
