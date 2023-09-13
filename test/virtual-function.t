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
return f.baz();
