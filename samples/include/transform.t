include "quaternion.t"

class Transform {
  static identity() : float<4,4> {
    return float<4,4>(float<4>(1.0, 0.0, 0.0, 0.0),
                      float<4>(0.0, 1.0, 0.0, 0.0),
                      float<4>(0.0, 0.0, 1.0, 0.0),
                      float<4>(0.0, 0.0, 0.0, 1.0));
  }
  static scale(x : float, y : float, z : float) : float<4,4> {
    return float<4,4>(float<4>(  x, 0.0, 0.0, 0.0),
                      float<4>(0.0,   y, 0.0, 0.0),
                      float<4>(0.0, 0.0,   z, 0.0),
                      float<4>(0.0, 0.0, 0.0, 1.0));
  }
  static translate(x : float, y : float, z : float) : float<4,4> {
    return float<4,4>(float<4>(1.0, 0.0, 0.0, 0.0),
                      float<4>(0.0, 1.0, 0.0, 0.0),
                      float<4>(0.0, 0.0, 1.0, 0.0),
                      float<4>(  x,   y,   z, 1.0));
  }
  static rotate(axis : float<3>, angle : float) : float<4,4> {
    var q = Quaternion(axis, angle);
    return q.toMatrix();
  }
  static projection(n : float, f : float, l : float, r : float, b : float, t : float) : float<4,4> {
    return float<4,4>(
      float<4>(2.0 * n / (r - l), 0.0, 0.0, 0.0),
      float<4>(0.0, 2.0 * n / (t - b), 0.0, 0.0),
      float<4>((r + l) / (r - l), (t + b) / (t - b), -(f + n) / (f - n), -1.0),
      float<4>(0.0, 0.0, -2.0 * f * n / (f - n), 0.0));
  }
  static perspective(fovy : float, aspect : float, n: float, f : float) : float<4,4> {
    var a = 1.0 / Math.tan(fovy / 2.0);
    return float<4,4>(
      float<4>(a / aspect, 0.0, 0.0,                   0.0),
      float<4>(0.0,        a,   0.0,                   0.0),
      float<4>(0.0,        0.0, (f + n) / (n - f),    -1.0),
      float<4>(0.0,        0.0, 2.0 * f * n / (n - f), 0.0));
  }
  static lookAt(eye : float<3>, center : float<3>, up : float<3>) : float<4,4> {
    var f = Math.normalize(center - eye);
    up = Math.normalize(up);
    var s = Math.normalize(Math.cross(f, up));
    var u = Math.cross(s, f);
    var t = float<3>(Math.dot(s, -eye), Math.dot(u, -eye), Math.dot(f, eye));
    return float<4,4>(
      float<4>(s.x, u.x, -f.x, 0.0),
      float<4>(s.y, u.y, -f.y, 0.0),
      float<4>(s.z, u.z, -f.z, 0.0),
      float<4>(t.x, t.y,  t.z, 1.0));
  }
  static swapRows(m : float<4,4>, i : int, j : int) : float<4,4> {
    for (var k = 0; k < 4; ++k) {
      var tmp = m[k][i];
      m[k][i] = m[k][j];
      m[k][j] = tmp;
    }
    return m;
  }
  static invert(matrix : float<4,4>) : float<4,4> {
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
