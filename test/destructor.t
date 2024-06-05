include "include/test.t"

class Foo {
  float r;
};
class Bar {
  Bar(Foo* f) {
    foo = f;
  }
 virtual ~Bar() {
    foo.r = 1234.0;
  }
  Foo* foo;
};
float r = -1.0;
{
  Foo* f = new Foo();
  f.r = -1.0;
  {
    Bar* b = new Bar(f);
  }
  r = f.r;
}
Test.Expect(r == 1234.0);
return r;
