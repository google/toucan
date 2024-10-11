include "include/test.t"

class C {
  C(_x : float, _y : float) : { x = _x + 5.0, y = _y + 2.0 } {}
  C(_x : float) : C(_x, _x) {}
  var x : float, y : float;
}

var c = C(40.0);
Test.Expect(c.x == 45.0);
Test.Expect(c.y == 42.0);
