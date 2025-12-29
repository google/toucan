class Bar {
  static Foo(i : int, f : float) : float {
    return f - i as float;
  }
}

var r = Bar.Foo(f = 3.0);
