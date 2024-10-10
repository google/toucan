class Cubic<T> {
  void FromBezier(T[4] p) {
    a =        p[0];
    b = -3.0 * p[0] + 3.0 * p[1];
    c =  3.0 * p[0] - 6.0 * p[1] + 3.0 * p[2];
    d =       -p[0] + 3.0 * p[1] - 3.0 * p[2] + p[3];
  }

  T Evaluate(float p) {
    return a + p * (b + p * (c + p * d));
  }

  T EvaluateTangent(float p) {
    return b + p * (2.0 * c + 3.0 * p * d);
  }

  var a : T, b : T, c : T, d : T;
}
