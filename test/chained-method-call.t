include "include/test.t"

class Foo {
  var value : float;
  Foo plusOne() {
    var f : Foo;
    f.value = value + 1.0;
    return f;
  }
  static Foo zero() {
    var f : Foo;
    f.value = 0.0;
    return f;
  }
}

var f = Foo.zero().plusOne();
Test.Expect(f.value == 1.0);
