include "include/test.t"

class C {
  static float M(float<4> v) {
    return v.x;
  }
}

float r = C.M({3.0, 0.0, 1.0, 2.0});
Test.Expect(r == 3.0);
