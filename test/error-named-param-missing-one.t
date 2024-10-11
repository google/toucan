class Bar {
  static Foo(i : int, f : float) : float {
    return f - (float) i;
  }
}

var r = Bar.Foo(f = 3.0);
