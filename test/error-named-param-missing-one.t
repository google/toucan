class Bar {
  static float Foo(int i, float f) {
    return f - (float) i;
  }
}

float r = Bar.Foo(f = 3.0);
return 0.0;
