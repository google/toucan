include "include/test.t"

class C {
  float f;
  float<4> v;
}

C c = { 0.0, { 1.0, 2.0, 3.0, 4.0 } };

Test.Expect(c.f == 0.0);
Test.Expect(c.v.x == 1.0);
Test.Expect(c.v.y == 2.0);
Test.Expect(c.v.z == 3.0);
Test.Expect(c.v.w == 4.0);
