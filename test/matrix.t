include "include/test.t"

float<4, 4> m;
m[3].w = 432.0;
m[1][0] = 123.0;

Test.Expect(m[3].w == 432.0);
Test.Expect(m[1][0] == 123.0);
