include "include/test.t"

class Foo {
  static float cast(int x) {
    var y = 0.0;
    while (x > 0) {
      y += 1.0;
      --x;
    }
    return y;
  }
};

var foo = new Foo();
Test.Expect(foo.cast(42) == 42.0);
