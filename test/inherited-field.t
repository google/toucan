include "include/test.t"

class Foo {
  var f : float;
};
class Bar : Foo {
  var g : float;
  sum() : float {
    return f + g;
  }
};
var b = new Bar;
b.f = 3.0;
b.g = 2.0;
Test.Expect(b.sum() == 5.0);
