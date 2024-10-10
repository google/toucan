include "include/test.t"
class Foo {
  var value : float;
  static Foo One() {
    var f = Foo{};
    f.value = 1.0;
    return f;
  }
}

Test.Expect(Foo.One().value == 1.0);
