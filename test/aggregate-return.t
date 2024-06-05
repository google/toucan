include "include/test.t"
class Foo {
  float value;
  static Foo One() {
    Foo f;
    f.value = 1.0;
    return f;
  }
}

Test.Expect(Foo.One().value == 1.0);
return Foo.One().value;
