include "include/test.t"
var m = float<4,4>{float<4>{1.0, 0.0, 0.0, 0.0},
                   float<4>{0.0, 1.0, 0.0, 0.0},
                   float<4>{0.0, 0.0, 1.0, 0.0},
                   float<4>{5.0,-3.0, 1.0, 1.0}};
Test.Expect(m[0][0] == 1.0);
Test.Expect(m[0][1] == 0.0);
Test.Expect(m[3][0] == 5.0);
Test.Expect(m[3][1] == -3.0);
Test.Expect(m[3][2] == 1.0);
Test.Expect(m[3][3] == 1.0);

var v = int<3>(3, 2, 1);
var m2 = int<3,2>(v);

Test.Expect(m2[0][0] == 3);
Test.Expect(m2[0][1] == 2);
Test.Expect(m2[0][2] == 1);
Test.Expect(m2[1][0] == 3);
Test.Expect(m2[1][1] == 2);
Test.Expect(m2[1][2] == 1);
