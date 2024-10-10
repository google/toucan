class Utils {
  static dot(float<2> v1, float<2> v2) : float {
    var r = v1 * v2;
    return r.x + r.y;
  }
  static dot(float<3> v1, float<3> v2) : float {
    var r = v1 * v2;
    return r.x + r.y + r.z;
  }
  static dot(float<4> v1, float<4> v2) : float {
    var r = v1 * v2;
    return r.x + r.y + r.z + r.w;
  }
  static length(float<2> v) : float {
    return Math.sqrt(Utils.dot(v, v));
  }
  static length(float<3> v) : float {
    return Math.sqrt(Utils.dot(v, v));
  }
  static makeFloat4(float<2> v) : float<4> {
    return float<4>(v.x, v.y, 0.0, 1.0);
  }
  static makeFloat4(float<3> v) : float<4> {
    return float<4>(v.x, v.y, v.z, 1.0);
  }
  static makeVector(float x, float y, float z, float<2> placeholder) : float<2> {
    return float<2>(x, y);
  }
  static makeVector(float x, float y, float z, float<3> placeholder) : float<3> {
    return float<3>(x, y, z);
  }
  static cross(float<3> a, float<3> b) : float<3> {
    return float<3>(a.y * b.z - a.z * b.y,
                    a.z * b.x - a.x * b.z,
                    a.x * b.y - a.y * b.x);
  }
  static normalize(float<3> v) : float<3> {
    return v * (1.0 / Utils.length(v));
  }
}
