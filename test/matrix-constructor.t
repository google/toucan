include "include/test.t"
var m = float<4,4>(float<4>(1.0, 0.0, 0.0, 0.0),
                   float<4>(0.0, 1.0, 0.0, 0.0),
                   float<4>(0.0, 0.0, 1.0, 0.0),
                   float<4>(5.0,-3.0, 1.0, 1.0));

Test.Expect(m[0][0] == 1.0);
Test.Expect(m[0][1] == 0.0);
Test.Expect(m[3][0] == 5.0);
Test.Expect(m[3][1] == -3.0);
Test.Expect(m[3][2] == 1.0);
Test.Expect(m[3][3] == 1.0);
