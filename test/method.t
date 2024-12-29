include "include/test.t"

class Foo {
  inc() {
    x = x + 1.0;
    return;
  }
  var x : float;
};

var foo = new Foo;
foo.x = 0.0;
foo.inc();
foo.inc();
foo.inc();
Test.Expect(foo.x == 3.0);
