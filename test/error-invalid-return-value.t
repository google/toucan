class C {
  ReturnsWeakPtrToInt() : ^int {
    var a = 42;
    return &a;
  }

  ReturnsInt() : int {
    return 42.0;
  }

  ReturnsFloat() : float {
    return 42;
  }
}
