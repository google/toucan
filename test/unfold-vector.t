include "include/test.t"

var v2 = float<2>{1.0, 2.0};
var v3 = float<3>{@v2, 3.0};
var v4 = float<4>{@v3, 4.0};

Test.Expect(v4.x == 1.0);
Test.Expect(v4.y == 2.0);
Test.Expect(v4.z == 3.0);
Test.Expect(v4.w == 4.0);

class Funky {
  var i : int;
  var v : float<4>;
}

class C {
  static vecify(a : float, b : float, c : float, d : float) : float<4> {
    return float<4>{a, b, c, d};
  }
  static defunkify(f : Funky) {
  }
}

var a = C.vecify(1.0, @float<2>{2.0, 3.0}, 4.0);
Test.Expect(a.x == 1.0 && a.y == 2.0 && a.z == 3.0 && a.w == 4.0);

var b = C.vecify(1.0, @float<3>{2.0, 3.0, 4.0});
Test.Expect(b.x == 1.0 && b.y == 2.0 && b.z == 3.0 && b.w == 4.0);

var c = C.vecify(@float<2>{1.0, 2.0}, @float<2>{3.0, 4.0});
Test.Expect(c.x == 1.0 && c.y == 2.0 && c.z == 3.0 && c.w == 4.0);

C.defunkify({ 1, {@float<2>{1.0, 2.0}, @float<2>{3.0, 4.0} }} );
