include "include/test.t"

class C {
  C() {}
  C(_x : float, _y : float) : { y = _x, x = _y } {}
  var x : float = -1.0;
  var y : float = -2.0;
}

var c = C(21.0, 42.0);
Test.Expect(c.x == 42.0);
Test.Expect(c.y == 21.0);

var pc = new C(7.0, 14.0);
Test.Expect(pc.x == 14.0);
Test.Expect(pc.y == 7.0);

var ic = C{21.0, 42.0};
Test.Expect(ic.x == 21.0);
Test.Expect(ic.y == 42.0);

var dic = C{};
Test.Expect(dic.x == -1.0);
Test.Expect(dic.y == -2.0);

var pdic = new C();
Test.Expect(pdic.x == -1.0);
Test.Expect(pdic.y == -2.0);
