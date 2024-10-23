class Foo {
  virtual bar() {
  }
};
class Bar : Foo {
  bar() {
  }
};
var f = new Bar();
