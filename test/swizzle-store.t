include "include/test.t"

float<4> m;
m.y = 432.0;
Test.Expect(m.y == 432.0);
return m.y;
