include "include/test.t"

class Bar;
class Foo {
  Bar* bar;
};

class Bar {
  float baz;
};

Foo* foo = new Foo();
foo.bar = new Bar();
foo.bar.baz = 2.0;
Test.Expect(foo.bar.baz == 2.0);
return foo.bar.baz;
