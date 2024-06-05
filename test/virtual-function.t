include "include/test.t"

class Foo {
  virtual float baz() {
    return 0.0;
  }
};
class Bar : Foo {
  virtual float baz() {
    return 1.0;
  }
};
Foo* f = new Bar();
Test.Expect(f.baz() == 1.0);
return f.baz();
