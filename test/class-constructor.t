include "include/test.t"

class C {
  C(_x : float, _y : float) : { y = _x, x = _y } {}
  var x : float;
  var y : float;
}

var c = C(21.0, 42.0);
Test.Expect(c.x == 42.0);
Test.Expect(c.y == 21.0);
