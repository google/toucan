include "include/test.t"

class C {
  void clobber(C^ c)
  {
    f = c.f;
  }
  float f;
}

C c;
c.clobber({42.0});
Test.Expect(c.f == 42.0);
