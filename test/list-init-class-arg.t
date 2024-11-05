include "include/test.t"

class C {
  clobber(c : &C)
  {
    f = c.f;
  }
  var f : float;
}

var c : C;
c.clobber({42.0});
Test.Expect(c.f == 42.0);
