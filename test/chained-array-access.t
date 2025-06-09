include "include/test.t"

class Foo {
  var value : float;
  plusOne() : Foo {
    var f : Foo;
    f.value = value + 1.0;
    return f;
  }
  static zero() : [1]Foo {
    var f : [1]Foo;
    f[0].value = 0.0;
    return f;
  }
}

var f = Foo.zero()[0].plusOne();
Test.Expect(f.value == 1.0);
