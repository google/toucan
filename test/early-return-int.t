include "include/test.t"

class C {
  static m() : float {
    if (1 == 2) {
      return 0.0;
    }
    return 1.0;
  }
}
Test.Expect(C.m() == 1.0);
