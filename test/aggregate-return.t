include "include/test.t"
class Foo {
  var value : float;
  static One() : Foo {
    var f = Foo{};
    f.value = 1.0;
    return f;
  }
}

Test.Expect(Foo.One().value == 1.0);
