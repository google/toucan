class Utils {
  static float dot(float<2> v1, float<2> v2) {
    var r = v1 * v2;
    return r.x + r.y;
  }
  static float dot(float<3> v1, float<3> v2) {
    var r = v1 * v2;
    return r.x + r.y + r.z;
  }
  static float dot(float<4> v1, float<4> v2) {
    var r = v1 * v2;
    return r.x + r.y + r.z + r.w;
  }
  static float length(float<2> v) {
    return Math.sqrt(Utils.dot(v, v));
  }
  static float length(float<3> v) {
    return Math.sqrt(Utils.dot(v, v));
  }
  static float<4> makeFloat4(float<2> v) {
    return float<4>(v.x, v.y, 0.0, 1.0);
  }
  static float<4> makeFloat4(float<3> v) {
    return float<4>(v.x, v.y, v.z, 1.0);
  }
  static float<2> makeVector(float x, float y, float z, float<2> placeholder) {
    return float<2>(x, y);
  }
  static float<3> makeVector(float x, float y, float z, float<3> placeholder) {
    return float<3>(x, y, z);
  }
  static float<3> cross(float<3> a, float<3> b) {
    return float<3>(a.y * b.z - a.z * b.y,
                    a.z * b.x - a.x * b.z,
                    a.x * b.y - a.y * b.x);
  }
  static float<3> normalize(float<3> v) {
    return v * (1.0 / Utils.length(v));
  }
}
