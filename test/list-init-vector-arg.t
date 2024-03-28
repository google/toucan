class C {
  static float M(float<4> v) {
    return v.x;
  }
}

return C.M({3.0, 0.0, 1.0, 2.0});
