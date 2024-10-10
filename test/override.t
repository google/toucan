include "include/test.t"

class Foo {
  float func() {
    return 1234.0;
  }
};

class Bar : Foo {
  float func() {
    return 2345.0;
  }
};

var b = new Bar();
Test.Expect(b.func() == 2345.0);
