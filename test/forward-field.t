include "include/test.t"

class Foo {
  float bar() {
    return baz;
  }
  float baz;
};

Foo* foo = new Foo();
foo.baz = -321.0;
Test.Expect(foo.baz == -321.0);
