include "include/test.t"

class Foo {
  var f : float;
  var vec : float<4>;
};

var foo = new Foo;
foo.vec = float<4>(1.0, 2.0, 3.0, 4.0);
Test.Expect(foo.vec.x == 1.0);
Test.Expect(foo.vec.y == 2.0);
Test.Expect(foo.vec.z == 3.0);
Test.Expect(foo.vec.w == 4.0);
