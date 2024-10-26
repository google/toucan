class Cubic<T> {
  FromBezier(p : [4]T) {
    a =        p[0];
    b = -3.0 * p[0] + 3.0 * p[1];
    c =  3.0 * p[0] - 6.0 * p[1] + 3.0 * p[2];
    d =       -p[0] + 3.0 * p[1] - 3.0 * p[2] + p[3];
  }

  Evaluate(p : float) : T {
    return a + p * (b + p * (c + p * d));
  }

  EvaluateTangent(p : float) : T {
    return b + p * (2.0 * c + 3.0 * p * d);
  }

  var a : T, b : T, c : T, d : T;
}
