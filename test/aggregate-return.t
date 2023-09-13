class Foo {
  float value;
  static Foo One() {
    Foo f;
    f.value = 1.0;
    return f;
  }
}

return Foo.One().value;
