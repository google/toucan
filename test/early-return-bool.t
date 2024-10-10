include "include/test.t"

class C {
  static m() : float {
    if (true == false) {
      return 0.0;
    }
    return 1.0;
  }
}
Test.Expect(C.m() == 1.0);
