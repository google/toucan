include "include/test.t"

class C {
  const i = 42;
  Check() {
    Test.Expect(i == 42);
  }
}

Test.Expect(C.i == 42);

var c : C;
Test.Expect(c.i == 42);
c.Check();

class D : C {
  Check() {
    Test.Expect(i == 42);
  }
}

var d : D;
Test.Expect(d.i == 42);
d.Check();
