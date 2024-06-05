include "include/test.t"

class C {
  C(float v1, float v2) : { value2 = v1, value1 = v2 } {}
  float value1, value2;
}

C c = C(1.0, 2.0);
Test.Expect(c.value1 == 2.0);
Test.Expect(c.value2 == 1.0);
return c.value1;
