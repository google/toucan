include "include/test.t"

class C {
  int i;
  float a = 42.0;
};

C c = { 3, 5.0 };
Test.Expect(c.i == 3);
Test.Expect(c.a == 5.0);
