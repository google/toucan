class Bar {
  static Foo(int i, float f) : float {
    return f - (float) i;
  }
}

Bar.Foo(b = 3.0, a = 1);
