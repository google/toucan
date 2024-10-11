include "include/test.t"

class Foo {
  static whee(x : float, y : float) : float {
    return x * y;
  }
};

Test.Expect(Foo.whee(2.0, 3.0) == 6.0);
