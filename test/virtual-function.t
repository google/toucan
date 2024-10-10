include "include/test.t"

class Foo {
  virtual baz() : float {
    return 0.0;
  }
};
class Bar : Foo {
  virtual baz() : float {
    return 1.0;
  }
};
var f = new Bar();
Test.Expect(f.baz() == 1.0);
