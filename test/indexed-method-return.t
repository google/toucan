include "include/test.t"

class Foo {
  static generate() : *float[] {
    var r = new float[10];
    r[9] = 1234.0;
    return r;
  }
}

Test.Expect(Foo.generate()[9] == 1234.0);
