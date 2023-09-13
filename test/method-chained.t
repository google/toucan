class Bar {
  Bar() { x = 2.0; }
  float bump() {
    x += 1.0;
    return x;
  }
  float x;
}

class Foo {
  Bar* bar() {
    return new Bar();
  }
};

class Bump {
}

Foo* foo = new Foo();
return foo.bar().bump();
