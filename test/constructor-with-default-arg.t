include "include/test.t"

class C {
  C(v : float = 3.0) {
    value = v;
  }
  var value : float;
}

var c = new C();
Test.Expect(c.value == 3.0);
