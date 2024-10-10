include "include/test.t"

class Foo {
  static whee() : float {
    return -1234.0;
  }
};

Test.Expect(Foo.whee() == -1234.0);
