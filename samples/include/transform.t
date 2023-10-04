include "quaternion.t"

class Transform {
  static float<4,4> identity() {
    return float<4,4>(float<4>(1.0, 0.0, 0.0, 0.0),
                      float<4>(0.0, 1.0, 0.0, 0.0),
                      float<4>(0.0, 0.0, 1.0, 0.0),
                      float<4>(0.0, 0.0, 0.0, 1.0));
  }
  static float<4,4> scale(float x, float y, float z) {
    return float<4,4>(float<4>(  x, 0.0, 0.0, 0.0),
                      float<4>(0.0,   y, 0.0, 0.0),
                      float<4>(0.0, 0.0,   z, 0.0),
                      float<4>(0.0, 0.0, 0.0, 1.0));
  }
  static float<4,4> translate(float x, float y, float z) {
    return float<4,4>(float<4>(1.0, 0.0, 0.0, 0.0),
                      float<4>(0.0, 1.0, 0.0, 0.0),
                      float<4>(0.0, 0.0, 1.0, 0.0),
                      float<4>(  x,   y,   z, 1.0));
  }
  static float<4,4> rotate(float<3> axis, float angle) {
    Quaternion q = Quaternion(axis, angle);
    return q.toMatrix();
  }
  static float<4,4> projection(float n, float f, float l, float r, float b, float t) {
    return float<4,4>(
      float<4>(2.0 * n / (r - l), 0.0, 0.0, 0.0),
      float<4>(0.0, 2.0 * n / (t - b), 0.0, 0.0),
      float<4>((r + l) / (r - l), (t + b) / (t - b), -(f + n) / (f - n), -1.0),
      float<4>(0.0, 0.0, -2.0 * f * n / (f - n), 0.0));
  }
  static float<4,4> swapRows(float<4,4> m, int i, int j) {
    float tmp;

    for (int k = 0; k < 4; ++k) {
      tmp = m[k][i];
      m[k][i] = m[k][j];
      m[k][j] = tmp;
    }
    return m;
  }
  static float<4,4> invert(float<4,4> matrix) {
    float<4,4> a = matrix;
    float<4,4> b = Transform.identity();

    int  i, j, i1, k;

    for (j = 0; j < 4; ++j) {
      i1 = j;
      for (i = j + 1; i < 4; ++i)
        if (Math.abs(a[j][i]) > Math.abs(a[j][i1]))
          i1 = i;

      if (i1 != j) {
        a = Transform.swapRows(a, i1, j);
        b = Transform.swapRows(b, i1, j);
      }

      if (a[j][j] == 0.0) {
        return b;
      }

      float s = 1.0 / a[j][j];

      for (i = 0; i < 4; ++i) {
        b[i][j] *= s;
        a[i][j] *= s;
      }

      for (i = 0; i < 4; ++i) {
        if (i != j) {
          float t = a[j][i];
          for (k = 0; k < 4; ++k) {
            b[k][i] -= t * b[k][j];
            a[k][i] -= t * a[k][j];
          }
        }
      }
    }
    return b;
  }
}
