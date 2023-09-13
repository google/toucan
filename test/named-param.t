class Bar {
  static float Foo(int i, float f) {
    return f - (float) i;
  }
}

return Bar.Foo(f = 3.0, i = 1);
