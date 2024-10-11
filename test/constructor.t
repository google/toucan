include "include/test.t"

class C {
  C(v : float) {
    value = v;
  }
  var value : float;
}

var c = new C(3.14159);
Test.Expect(c.value == 3.14159);
