include "include/test.t"

class Foo {
  float value;
  Foo plusOne() {
    Foo f;
    f.value = value + 1.0;
    return f;
  }
  static Foo zero() {
    Foo f;
    f.value = 0.0;
    return f;
  }
}

Foo f = Foo.zero().plusOne();
Test.Expect(f.value == 1.0);
