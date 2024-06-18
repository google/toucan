include "include/test.t"

Test.Expect(Math.fabs(-3.0) == 3.0);
Test.Expect(Math.fabs(3.0) == 3.0);
Test.Expect(Math.fabs(0.0) == 0.0);
