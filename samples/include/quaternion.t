include "utils.t"

class Quaternion {
  Quaternion(float x, float y, float z, float w) { q = float<4>(x, y, z, w); }
  Quaternion(float<4> v) { q = v; }
  Quaternion(float<3> axis, float angle) {
    float<3> scaledAxis = axis * Math.sin(angle * 0.5);

    q.x = scaledAxis.x;
    q.y = scaledAxis.y;
    q.z = scaledAxis.z;
    q.w = Math.cos(angle * 0.5);
  }
  float len() { return Math.sqrt(Utils.dot(q, q)); }
  void normalize() { q = q / this.len(); }
  Quaternion mul(Quaternion other) {
    float<4> p = other.q;
    Quaternion r;

    r.q.x = p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y;
    r.q.y = p.w * q.y + p.y * q.w + p.z * q.x - p.x * q.z;
    r.q.z = p.w * q.z + p.z * q.w + p.x * q.y - p.y * q.x;
    r.q.w = p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z;

    return r;
  }
  float<4,4> toMatrix() {
    float x = q.x;
    float y = q.y;
    float z = q.z;
    float w = q.w;
    return float<4,4>(float<4>(1.0-2.0*(y*y+z*z),     2.0*(x*y+z*w),     2.0*(x*z-y*w), 0.0),
                      float<4>(    2.0*(x*y-z*w), 1.0-2.0*(x*x+z*z),     2.0*(y*z+x*w), 0.0),
                      float<4>(    2.0*(x*z+y*w),     2.0*(y*z-x*w), 1.0-2.0*(x*x+y*y), 0.0),
                      float<4>(              0.0,               0.0,               0.0, 1.0));
  }
  float<4> q;
}
