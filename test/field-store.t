include "include/test.t"

class Bar;
class Foo {
  var bar : *Bar;
};

class Bar {
  var baz : float;
};

var foo = new Foo();
foo.bar = new Bar();
foo.bar.baz = 2.0;
Test.Expect(foo.bar.baz == 2.0);
