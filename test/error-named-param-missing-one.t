class Bar {
  static float Foo(int i, float f) {
    return f - (float) i;
  }
}

var r = Bar.Foo(f = 3.0);
