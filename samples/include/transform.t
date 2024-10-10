include "quaternion.t"

class Transform {
  static identity() : float<4,4> {
    return float<4,4>(float<4>(1.0, 0.0, 0.0, 0.0),
                      float<4>(0.0, 1.0, 0.0, 0.0),
                      float<4>(0.0, 0.0, 1.0, 0.0),
                      float<4>(0.0, 0.0, 0.0, 1.0));
  }
  static scale(float x, float y, float z) : float<4,4> {
    return float<4,4>(float<4>(  x, 0.0, 0.0, 0.0),
                      float<4>(0.0,   y, 0.0, 0.0),
                      float<4>(0.0, 0.0,   z, 0.0),
                      float<4>(0.0, 0.0, 0.0, 1.0));
  }
  static translate(float x, float y, float z) : float<4,4> {
    return float<4,4>(float<4>(1.0, 0.0, 0.0, 0.0),
                      float<4>(0.0, 1.0, 0.0, 0.0),
                      float<4>(0.0, 0.0, 1.0, 0.0),
                      float<4>(  x,   y,   z, 1.0));
  }
  static rotate(float<3> axis, float angle) : float<4,4> {
    var q = Quaternion(axis, angle);
    return q.toMatrix();
  }
  static projection(float n, float f, float l, float r, float b, float t) : float<4,4> {
    return float<4,4>(
      float<4>(2.0 * n / (r - l), 0.0, 0.0, 0.0),
      float<4>(0.0, 2.0 * n / (t - b), 0.0, 0.0),
      float<4>((r + l) / (r - l), (t + b) / (t - b), -(f + n) / (f - n), -1.0),
      float<4>(0.0, 0.0, -2.0 * f * n / (f - n), 0.0));
  }
  static swapRows(float<4,4> m, int i, int j) : float<4,4> {
    for (var k = 0; k < 4; ++k) {
      var tmp = m[k][i];
      m[k][i] = m[k][j];
      m[k][j] = tmp;
    }
    return m;
  }
  static invert(float<4,4> matrix) : float<4,4> {
    var a = matrix;
    var b = Transform.identity();

    for (var j = 0; j < 4; ++j) {
      var i1 = j;
      for (var i = j + 1; i < 4; ++i)
        if (Math.fabs(a[j][i]) > Math.fabs(a[j][i1]))
          i1 = i;

      if (i1 != j) {
        a = Transform.swapRows(a, i1, j);
        b = Transform.swapRows(b, i1, j);
      }

      if (a[j][j] == 0.0) {
        return b;
      }

      var s = 1.0 / a[j][j];

      for (var i = 0; i < 4; ++i) {
        b[i][j] *= s;
        a[i][j] *= s;
      }

      for (var i = 0; i < 4; ++i) {
        if (i != j) {
          var t = a[j][i];
          for (var k = 0; k < 4; ++k) {
            b[k][i] -= t * b[k][j];
            a[k][i] -= t * a[k][j];
          }
        }
      }
    }
    return b;
  }
}
