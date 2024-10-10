include "include/test.t"

class Foo {
  float bar() {
    return baz;
  }
  var baz : float;
};

var foo = new Foo();
foo.baz = -321.0;
Test.Expect(foo.baz == -321.0);
