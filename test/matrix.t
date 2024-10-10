include "include/test.t"

var m : float<4, 4>;
m[3].w = 432.0;
m[1][0] = 123.0;

Test.Expect(m[3].w == 432.0);
Test.Expect(m[1][0] == 123.0);
