include "include/test.t"

class C {
  static float m() {
    if (1 == 2) {
      return 0.0;
    }
    return 1.0;
  }
}
Test.Expect(C.m() == 1.0);
