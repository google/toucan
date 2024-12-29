include "include/test.t"

class Foo {
  bar(a : int) : float {
    var f = 0.0;
    for (; a > 0; --a) {
      f += 1.0;
    }
    return f;
  }
  bar(a : float) : float {
    return 0.0-a;
  }
};

var f = new Foo;
Test.Expect(f.bar(3) == 3.0);
Test.Expect(f.bar(-4.0) == 4.0);
