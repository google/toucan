include "include/test.t"

class Foo {
  int x;
  float y;
};

Foo[]^ foo = new Foo[100];
foo[23].y = 5.0;
Test.Expect(foo[23].y == 5.0);
return foo[23].y;
