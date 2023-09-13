class Foo {
  virtual void bar() {
  }
  float baz() {
    return 0.0;
  }
};
class Bar : Foo {
  void bar() {
  }
  virtual float baz() {
    return 1.0;
  }
};
Foo* f = new Bar();
return f.baz();
