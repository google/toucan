include "include/test.t"

class C {
  C(v1 : float, v2 : float) : { value2 = v1, value1 = v2 } {}
  var value1 : float, value2 : float;
}

var c = C(1.0, 2.0);
Test.Expect(c.value1 == 2.0);
Test.Expect(c.value2 == 1.0);
