include "include/test.t"

class C {
  int i;
  float a = 42.0;

};

class D {
  C c;
  float f;
}

D d = { { 3, 5.0 }, 2.0 };
Test.Expect(d.c.i == 3);
Test.Expect(d.c.a == 5.0);
Test.Expect(d.f == 2.0);
return d.c.a + (float) d.c.i - d.f;
