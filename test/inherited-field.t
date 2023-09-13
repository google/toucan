class Foo {
  float f;
};
class Bar : Foo {
  float g;
  float sum() {
    return f + g;
  }
};
Bar* b = new Bar();
b.f = 3.0;
b.g = 2.0;
return b.sum();
