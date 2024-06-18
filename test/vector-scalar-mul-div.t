include "include/test.t"

float<2> x = float<2>(1.0, 2.0) * 2.0;
float<2> y = float<2>(3.0, 4.0) / 2.0;

Test.Expect(x.x == 2.0);
Test.Expect(x.y == 4.0);
Test.Expect(y.x == 1.5);
Test.Expect(y.y == 2.0);
