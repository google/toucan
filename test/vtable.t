include "include/test.t"

class Foo {
  virtual void dummy() {}
  virtual float baz() {
    return 0.0;
  }
};
class Bar : Foo {
  virtual float baz() {
    return 1.0;
  }
};
var f = new Bar();
Test.Expect(f.baz() == 1.0);
