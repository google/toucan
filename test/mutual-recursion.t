include "include/test.t"

class Foo {
  var y : float;
  foo(x : int) {
    if (x > 0) this.bar(x - 1);
    return;
  }
  bar(x : int) {
    y += 1.0;
    this.foo(x);
    return;
  }
};

var foo = new Foo;
foo.y = 0.0;
foo.foo(10);
Test.Expect(foo.y == 10.0);

class Bar {
  var y : float;
  foo(x : int, y = 0) {
    if (x > 0) this.bar(x - 1, 1.0);
    return;
  }
  bar(x : int, z = 0.0) {
    y += 1.0;
    this.foo(x, 42);
    return;
  }
};

var bar = new Bar;
bar.y = 0.0;
bar.foo(10);
Test.Expect(bar.y == 10.0);
