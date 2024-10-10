include "include/test.t"

var m : float<4>;
m[1] = 432.0;

Test.Expect(m[1] == 432.0);
