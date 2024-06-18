include "include/test.t"

class Foo {
  int[3] zzz;
  float a;
};
float r;
{
  Foo* f = new Foo();
  f.a = -1234.0;
  r = f.a;
}
Test.Expect(r == -1234.0);
