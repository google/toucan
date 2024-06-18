include "include/test.t"

class Foo {
  static float whee() {
    return -1234.0;
  }
};

Test.Expect(Foo.whee() == -1234.0);
