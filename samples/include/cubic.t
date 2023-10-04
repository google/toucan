class Cubic<T> {
  void FromBezier(T p0, T p1, T p2, T p3) {
    a =        p0;
    b = -3.0 * p0 + 3.0 * p1;
    c =  3.0 * p0 - 6.0 * p1 + 3.0 * p2;
    d =       -p0 + 3.0 * p1 - 3.0 * p2 + p3;
  }

  T Evaluate(float p) {
    return a + p * (b + p * (c + p * d));
  }

  T EvaluateTangent(float p) {
    return b + p * (2.0 * c + 3.0 * p * d);
  }

  T a, b, c, d;
}
