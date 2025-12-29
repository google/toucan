include "include/test.t"

class Bar {
  static Foo(i : int = -5, f : float) : float {
    return f - i as float;
  }
}

Test.Expect(Bar.Foo(f = 3.0) == 8.0);
