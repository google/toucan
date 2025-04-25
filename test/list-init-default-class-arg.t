include "include/test.t"

class C {
  var i = 42;
}

class D {
  static RunTest(c : &C = {}) {
    Test.Expect(c.i == 42);
  }
}

D.RunTest();
