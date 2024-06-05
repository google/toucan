include "include/test.t"

class Foo {
  int i;
  float f = 42.0;
};

Foo foo;
Test.Expect(foo.f == 42.0);
return foo.f;
