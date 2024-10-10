include "include/test.t"

class C {
  void clobber(C^ c)
  {
    f = c.f;
  }
  var f : float;
}

var c : C;
c.clobber({42.0});
Test.Expect(c.f == 42.0);
