class Bar {
  static float Foo(int i = -5, float f) {
    return f - (float) i;
  }
}

return Bar.Foo(f = 3.0);
