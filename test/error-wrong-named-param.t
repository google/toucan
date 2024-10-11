class Bar {
  static Foo(i : int, f : float) : float {
    return f - (float) i;
  }
}

Bar.Foo(b = 3.0, a = 1);
