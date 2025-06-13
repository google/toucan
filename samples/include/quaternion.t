class Quaternion {
  Quaternion(x : float, y : float, z : float, w : float) { q = float<4>(x, y, z, w); }
  Quaternion(v : float<4>) { q = v; }
  Quaternion(axis : float<3>, angle : float) {
    var scaledAxis = axis * Math.sin(angle * 0.5);

    q.xyz = scaledAxis;
    q.w = Math.cos(angle * 0.5);
  }
  len() : float { return Math.length(q); }
  normalize() { q = q / this.len(); }
  mul(other : Quaternion) : Quaternion {
    var p = other.q;
    var r : Quaternion;

    r.q.x = p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y;
    r.q.y = p.w * q.y + p.y * q.w + p.z * q.x - p.x * q.z;
    r.q.z = p.w * q.z + p.z * q.w + p.x * q.y - p.y * q.x;
    r.q.w = p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z;

    return r;
  }
  toMatrix() : float<4,4> {
    var x = q.x;
    var y = q.y;
    var z = q.z;
    var w = q.w;
    return float<4,4>(float<4>(1.0-2.0*(y*y+z*z),     2.0*(x*y+z*w),     2.0*(x*z-y*w), 0.0),
                      float<4>(    2.0*(x*y-z*w), 1.0-2.0*(x*x+z*z),     2.0*(y*z+x*w), 0.0),
                      float<4>(    2.0*(x*z+y*w),     2.0*(y*z-x*w), 1.0-2.0*(x*x+y*y), 0.0),
                      float<4>(              0.0,               0.0,               0.0, 1.0));
  }
  var q : float<4>;
}
