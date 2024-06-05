include "include/test.t"

float<4> m;
m[1] = 432.0;

Test.Expect(m[1] == 432.0);
return m[1];
