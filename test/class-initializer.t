include "include/test.t"

class C {
  C(float _x, float _y) : { y = _x, x = _y } {}
  var x : float;
  var y : float;
}

var c = C{21.0, 42.0};
Test.Expect(c.x == 21.0);
Test.Expect(c.y == 42.0);
