class Bar {
  static float Foo(int i, float f) {
    return f - (float) i;
  }
}

return Bar.Foo(b = 3.0, a = 1);
