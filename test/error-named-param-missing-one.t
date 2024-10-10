class Bar {
  static Foo(int i, float f) : float {
    return f - (float) i;
  }
}

var r = Bar.Foo(f = 3.0);
