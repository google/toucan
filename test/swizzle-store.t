include "include/test.t"

var m : float<4>;
m.y = 432.0;
Test.Expect(m.y == 432.0);
