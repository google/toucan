include "include/test.t"

class Foo {
  static float[]* generate() {
    float[]* r = new float[10];
    r[9] = 1234.0;
    return r;
  }
}

Test.Expect(Foo.generate()[9] == 1234.0);
return Foo.generate()[9];
