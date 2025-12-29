include "include/test.t"

class Bar {
  static Foo(i : int, f : float) : float {
    return f - i as float;
  }
}

Test.Expect(Bar.Foo(f = 3.0, i = 1) == 2.0);
