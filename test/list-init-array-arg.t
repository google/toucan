include "include/test.t"

class C {
  static pf(a : &[3]int) {
    Test.Expect(a[0] == 3);
    Test.Expect(a[1] == 2);
    Test.Expect(a[2] == 1);
  }
  static pv(a : &[]int) {
    Test.Expect(a[0] == 3);
    Test.Expect(a[1] == 2);
    Test.Expect(a[2] == 1);
  }
}

C.pf({3, 2, 1});
C.pv({3, 2, 1});
