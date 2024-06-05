include "include/test.t"

float<2> v21 = float<2>(1.0);
float<2> v22 = float<2>(2.0, 3.0);

Test.Expect(v21.x == 1.0);
Test.Expect(v21.y == 1.0);
Test.Expect(v22.x == 2.0);
Test.Expect(v22.y == 3.0);

return v21.x + v21.y + v22.x + v22.y;
