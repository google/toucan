include "include/test.t"

class Foo {
  static generate() : *[]float {
    var r = [10] new float;
    r[9] = 1234.0;
    return r;
  }
}

Test.Expect(Foo.generate()[9] == 1234.0);
