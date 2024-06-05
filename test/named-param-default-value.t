include "include/test.t"

class Bar {
  static float Foo(int i = -5, float f) {
    return f - (float) i;
  }
}

Test.Expect(Bar.Foo(f = 3.0) == 8.0);
return Bar.Foo(f = 3.0);
