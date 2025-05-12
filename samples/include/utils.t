class Utils {
  static makeFloat4(v : float<2>) : float<4> {
    return float<4>(v.x, v.y, 0.0, 1.0);
  }
  static makeFloat4(v : float<3>) : float<4> {
    return float<4>(v.x, v.y, v.z, 1.0);
  }
  static makeVector(x : float, y : float, z : float, placeholder : float<2>) : float<2> {
    return float<2>(x, y);
  }
  static makeVector(x : float, y : float, z : float, placeholder : float<3>) : float<3> {
    return float<3>(x, y, z);
  }
}
