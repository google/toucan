include "include/test.t"

class Foo {
  void inc() {
    x = x + 1.0;
    return;
  }
  float x;
};

Foo foo;
foo.x = 0.0;
foo.inc();
foo.inc();
foo.inc();
Test.Expect(foo.x == 3.0);
return foo.x;
