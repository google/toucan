include "include/test.t"

class Foo {
  var value : float;
  plusOne() : Foo {
    var f : Foo;
    f.value = value + 1.0;
    return f;
  }
  static zero() : Foo {
    var f : Foo;
    f.value = 0.0;
    return f;
  }
}

var f = Foo.zero().plusOne();
Test.Expect(f.value == 1.0);
