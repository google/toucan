include "include/test.t"

var v21 : float<2> = {1.0, 0.0};
var v22 : float<2> = {2.0, 3.0};

var v : int<4> = {1, 2, 3, 4};
v = {1, 1, 1, 1};

Test.Expect(v21.x == 1.0);
Test.Expect(v21.y == 0.0);
Test.Expect(v22.x == 2.0);
Test.Expect(v22.y == 3.0);
Test.Expect(v.x == 1);
Test.Expect(v.y == 1);
Test.Expect(v.z == 1);
Test.Expect(v.w == 1);
