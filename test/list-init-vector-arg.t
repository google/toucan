include "include/test.t"

class C {
  static M(v : float<4>) : float {
    return v.x;
  }
}

var r = C.M({3.0, 0.0, 1.0, 2.0});
Test.Expect(r == 3.0);
