include "include/test.t"

class Bar;

class Foo {
  Foo(bar : ^Bar) : { bar = bar } {
    bar.count++;
  }
 ~Foo() {
    bar.count--;
  }

  var bar : ^Bar;
};

class Bar {
  var count : int = 0;
};

var bar = new Bar();
bar.count = 0;
Test.Expect(bar.count == 0);
var foo = new Foo(bar);
Test.Expect(bar.count == 1);
foo = null;
Test.Expect(bar.count == 0);
