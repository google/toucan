include "include/test.t"

class Bar {
  static Foo(i : int, f : float) : float {
    return f - (float) i;
  }
}

Test.Expect(Bar.Foo(f = 3.0, i = 1) == 2.0);
