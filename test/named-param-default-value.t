include "include/test.t"

class Bar {
  static Foo(int i = -5, float f) : float {
    return f - (float) i;
  }
}

Test.Expect(Bar.Foo(f = 3.0) == 8.0);
