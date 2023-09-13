class Vertex {
  Vertex(float<3> p, float<3> n) { position = p; normal = n; }
  float<3> position;
  float<3> normal;
}

class Utils {
  static float dot(float<4> v1, float<4> v2) {
    float<4> r = v1 * v2;
    return r.x + r.y + r.z + r.w;
  }
}

class Cubic<T> {
  void FromBezier(T p0, T p1, T p2, T p3) {
    a =        p0;
    b = -3.0 * p0 + 3.0 * p1;
    c =  3.0 * p0 - 6.0 * p1 + 3.0 * p2;
    d =       -p0 + 3.0 * p1 - 3.0 * p2 + p3;
  }

  T Evaluate(float p) {
    return a + p * (b + p * (c + p * d));
  }

  T EvaluateTangent(float p) {
    return b + p * (2.0 * c + 3.0 * p * d);
  }

  T a, b, c, d;
}

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

using Format = RGBA8unorm;

class Loader {
  static void Load(Device* device, ubyte[]^ data, Texture2D<Format>^ texture, uint layer) {
    auto image = new ImageDecoder<Format>(data);
    auto buffer = new Buffer<Format::MemoryType[]>(device, texture.MinBufferWidth() * image.Height());
    writeonly Format::MemoryType[]^ b = buffer.MapWrite();
    image.Decode(b, texture.MinBufferWidth());
    buffer.Unmap();
    CommandEncoder* encoder = new CommandEncoder(device);
    texture.CopyFromBuffer(encoder, buffer, image.Width(), image.Height(), 1, uint<3>(0, 0, layer));
    device.GetQueue().Submit(encoder.Finish());
  }
}

Device* device = new Device();

auto sizer = new ImageDecoder<Format>(inline("third_party/home-cube/right.jpg"));
auto texture = new sampled Texture2D<RGBA8unorm>(device, sizer.Width(), sizer.Height(), 6);
Loader.Load(device, inline("third_party/home-cube/right.jpg"), texture, 0);
Loader.Load(device, inline("third_party/home-cube/left.jpg"), texture, 1);
Loader.Load(device, inline("third_party/home-cube/top.jpg"), texture, 2);
Loader.Load(device, inline("third_party/home-cube/bottom.jpg"), texture, 3);
Loader.Load(device, inline("third_party/home-cube/front.jpg"), texture, 4);
Loader.Load(device, inline("third_party/home-cube/back.jpg"), texture, 5);

Window* window = new Window(device, 0, 0, 1024, 1024);
SwapChain* swapChain = new SwapChain(window);

auto cubeVerts = float<3>[24](
  // Right face
  float<3>( 1.0, -1.0, -1.0),
  float<3>( 1.0, -1.0,  1.0),
  float<3>( 1.0,  1.0,  1.0),
  float<3>( 1.0,  1.0, -1.0),

  // Left face
  float<3>(-1.0, -1.0, -1.0),
  float<3>(-1.0,  1.0, -1.0),
  float<3>(-1.0,  1.0,  1.0),
  float<3>(-1.0, -1.0,  1.0),

  // Bottom face
  float<3>(-1.0, -1.0, -1.0),
  float<3>( 1.0, -1.0, -1.0),
  float<3>( 1.0, -1.0,  1.0),
  float<3>(-1.0, -1.0,  1.0),

  // Top face
  float<3>(-1.0,  1.0, -1.0),
  float<3>(-1.0,  1.0,  1.0),
  float<3>( 1.0,  1.0,  1.0),
  float<3>( 1.0,  1.0, -1.0),

  // Front face
  float<3>(-1.0, -1.0, -1.0),
  float<3>( 1.0, -1.0, -1.0),
  float<3>( 1.0,  1.0, -1.0),
  float<3>(-1.0,  1.0, -1.0),

  // Back face
  float<3>(-1.0, -1.0,  1.0),
  float<3>( 1.0, -1.0,  1.0),
  float<3>( 1.0,  1.0,  1.0),
  float<3>(-1.0,  1.0,  1.0)
);

auto cubeIndices = uint[36](
  0,  1,   2,  0,  2,  3,
  4,  5,   6,  4,  6,  7,
  8,  9,  10,  8, 10, 11,
  12, 13, 14, 12, 14, 15,
  16, 17, 18, 16, 18, 19,
  20, 21, 22, 20, 22, 23
);

auto teapotControlPoints = float<3>[290](
  float<3>(1.4, 0.0, 2.4),
  float<3>(1.4, -0.784, 2.4),
  float<3>(0.784, -1.4, 2.4),
  float<3>(0.0, -1.4, 2.4),
  float<3>(1.3375, 0.0, 2.53125),
  float<3>(1.3375, -0.749, 2.53125),
  float<3>(0.749, -1.3375, 2.53125),
  float<3>(0.0, -1.3375, 2.53125),
  float<3>(1.4375, 0.0, 2.53125),
  float<3>(1.4375, -0.805, 2.53125),
  float<3>(0.805, -1.4375, 2.53125),
  float<3>(0.0, -1.4375, 2.53125),
  float<3>(1.5, 0.0, 2.4),
  float<3>(1.5, -0.84, 2.4),
  float<3>(0.84, -1.5, 2.4),
  float<3>(0.0, -1.5, 2.4),
  float<3>(-0.784, -1.4, 2.4),
  float<3>(-1.4, -0.784, 2.4),
  float<3>(-1.4, 0.0, 2.4),
  float<3>(-0.749, -1.3375, 2.53125),
  float<3>(-1.3375, -0.749, 2.53125),
  float<3>(-1.3375, 0.0, 2.53125),
  float<3>(-0.805, -1.4375, 2.53125),
  float<3>(-1.4375, -0.805, 2.53125),
  float<3>(-1.4375, 0.0, 2.53125),
  float<3>(-0.84, -1.5, 2.4),
  float<3>(-1.5, -0.84, 2.4),
  float<3>(-1.5, 0.0, 2.4),
  float<3>(-1.4, 0.784, 2.4),
  float<3>(-0.784, 1.4, 2.4),
  float<3>(0.0, 1.4, 2.4),
  float<3>(-1.3375, 0.749, 2.53125),
  float<3>(-0.749, 1.3375, 2.53125),
  float<3>(0.0, 1.3375, 2.53125),
  float<3>(-1.4375, 0.805, 2.53125),
  float<3>(-0.805, 1.4375, 2.53125),
  float<3>(0.0, 1.4375, 2.53125),
  float<3>(-1.5, 0.84, 2.4),
  float<3>(-0.84, 1.5, 2.4),
  float<3>(0.0, 1.5, 2.4),
  float<3>(0.784, 1.4, 2.4),
  float<3>(1.4, 0.784, 2.4),
  float<3>(0.749, 1.3375, 2.53125),
  float<3>(1.3375, 0.749, 2.53125),
  float<3>(0.805, 1.4375, 2.53125),
  float<3>(1.4375, 0.805, 2.53125),
  float<3>(0.84, 1.5, 2.4),
  float<3>(1.5, 0.84, 2.4),
  float<3>(1.75, 0.0, 1.875),
  float<3>(1.75, -0.98, 1.875),
  float<3>(0.98, -1.75, 1.875),
  float<3>(0.0, -1.75, 1.875),
  float<3>(2.0, 0.0, 1.35),
  float<3>(2.0, -1.12, 1.35),
  float<3>(1.12, -2.0, 1.35),
  float<3>(0.0, -2.0, 1.35),
  float<3>(2.0, 0.0, 0.9),
  float<3>(2.0, -1.12, 0.9),
  float<3>(1.12, -2.0, 0.9),
  float<3>(0.0, -2.0, 0.9),
  float<3>(-0.98, -1.75, 1.875),
  float<3>(-1.75, -0.98, 1.875),
  float<3>(-1.75, 0.0, 1.875),
  float<3>(-1.12, -2.0, 1.35),
  float<3>(-2.0, -1.12, 1.35),
  float<3>(-2.0, 0.0, 1.35),
  float<3>(-1.12, -2.0, 0.9),
  float<3>(-2.0, -1.12, 0.9),
  float<3>(-2.0, 0.0, 0.9),
  float<3>(-1.75, 0.98, 1.875),
  float<3>(-0.98, 1.75, 1.875),
  float<3>(0.0, 1.75, 1.875),
  float<3>(-2.0, 1.12, 1.35),
  float<3>(-1.12, 2.0, 1.35),
  float<3>(0.0, 2.0, 1.35),
  float<3>(-2.0, 1.12, 0.9),
  float<3>(-1.12, 2.0, 0.9),
  float<3>(0.0, 2.0, 0.9),
  float<3>(0.98, 1.75, 1.875),
  float<3>(1.75, 0.98, 1.875),
  float<3>(1.12, 2.0, 1.35),
  float<3>(2.0, 1.12, 1.35),
  float<3>(1.12, 2.0, 0.9),
  float<3>(2.0, 1.12, 0.9),
  float<3>(2.0, 0.0, 0.45),
  float<3>(2.0, -1.12, 0.45),
  float<3>(1.12, -2.0, 0.45),
  float<3>(0.0, -2.0, 0.45),
  float<3>(1.5, 0.0, 0.225),
  float<3>(1.5, -0.84, 0.225),
  float<3>(0.84, -1.5, 0.225),
  float<3>(0.0, -1.5, 0.225),
  float<3>(1.5, 0.0, 0.15),
  float<3>(1.5, -0.84, 0.15),
  float<3>(0.84, -1.5, 0.15),
  float<3>(0.0, -1.5, 0.15),
  float<3>(-1.12, -2.0, 0.45),
  float<3>(-2.0, -1.12, 0.45),
  float<3>(-2.0, 0.0, 0.45),
  float<3>(-0.84, -1.5, 0.225),
  float<3>(-1.5, -0.84, 0.225),
  float<3>(-1.5, 0.0, 0.225),
  float<3>(-0.84, -1.5, 0.15),
  float<3>(-1.5, -0.84, 0.15),
  float<3>(-1.5, 0.0, 0.15),
  float<3>(-2.0, 1.12, 0.45),
  float<3>(-1.12, 2.0, 0.45),
  float<3>(0.0, 2.0, 0.45),
  float<3>(-1.5, 0.84, 0.225),
  float<3>(-0.84, 1.5, 0.225),
  float<3>(0.0, 1.5, 0.225),
  float<3>(-1.5, 0.84, 0.15),
  float<3>(-0.84, 1.5, 0.15),
  float<3>(0.0, 1.5, 0.15),
  float<3>(1.12, 2.0, 0.45),
  float<3>(2.0, 1.12, 0.45),
  float<3>(0.84, 1.5, 0.225),
  float<3>(1.5, 0.84, 0.225),
  float<3>(0.84, 1.5, 0.15),
  float<3>(1.5, 0.84, 0.15),
  float<3>(-1.6, 0.0, 2.025),
  float<3>(-1.6, -0.3, 2.025),
  float<3>(-1.5, -0.3, 2.25),
  float<3>(-1.5, 0.0, 2.25),
  float<3>(-2.3, 0.0, 2.025),
  float<3>(-2.3, -0.3, 2.025),
  float<3>(-2.5, -0.3, 2.25),
  float<3>(-2.5, 0.0, 2.25),
  float<3>(-2.7, 0.0, 2.025),
  float<3>(-2.7, -0.3, 2.025),
  float<3>(-3.0, -0.3, 2.25),
  float<3>(-3.0, 0.0, 2.25),
  float<3>(-2.7, 0.0, 1.8),
  float<3>(-2.7, -0.3, 1.8),
  float<3>(-3.0, -0.3, 1.8),
  float<3>(-3.0, 0.0, 1.8),
  float<3>(-1.5, 0.3, 2.25),
  float<3>(-1.6, 0.3, 2.025),
  float<3>(-2.5, 0.3, 2.25),
  float<3>(-2.3, 0.3, 2.025),
  float<3>(-3.0, 0.3, 2.25),
  float<3>(-2.7, 0.3, 2.025),
  float<3>(-3.0, 0.3, 1.8),
  float<3>(-2.7, 0.3, 1.8),
  float<3>(-2.7, 0.0, 1.575),
  float<3>(-2.7, -0.3, 1.575),
  float<3>(-3.0, -0.3, 1.35),
  float<3>(-3.0, 0.0, 1.35),
  float<3>(-2.5, 0.0, 1.125),
  float<3>(-2.5, -0.3, 1.125),
  float<3>(-2.65, -0.3, 0.9375),
  float<3>(-2.65, 0.0, 0.9375),
  float<3>(-2.0, -0.3, 0.9),
  float<3>(-1.9, -0.3, 0.6),
  float<3>(-1.9, 0.0, 0.6),
  float<3>(-3.0, 0.3, 1.35),
  float<3>(-2.7, 0.3, 1.575),
  float<3>(-2.65, 0.3, 0.9375),
  float<3>(-2.5, 0.3, 1.125),
  float<3>(-1.9, 0.3, 0.6),
  float<3>(-2.0, 0.3, 0.9),
  float<3>(1.7, 0.0, 1.425),
  float<3>(1.7, -0.66, 1.425),
  float<3>(1.7, -0.66, 0.6),
  float<3>(1.7, 0.0, 0.6),
  float<3>(2.6, 0.0, 1.425),
  float<3>(2.6, -0.66, 1.425),
  float<3>(3.1, -0.66, 0.825),
  float<3>(3.1, 0.0, 0.825),
  float<3>(2.3, 0.0, 2.1),
  float<3>(2.3, -0.25, 2.1),
  float<3>(2.4, -0.25, 2.025),
  float<3>(2.4, 0.0, 2.025),
  float<3>(2.7, 0.0, 2.4),
  float<3>(2.7, -0.25, 2.4),
  float<3>(3.3, -0.25, 2.4),
  float<3>(3.3, 0.0, 2.4),
  float<3>(1.7, 0.66, 0.6),
  float<3>(1.7, 0.66, 1.425),
  float<3>(3.1, 0.66, 0.825),
  float<3>(2.6, 0.66, 1.425),
  float<3>(2.4, 0.25, 2.025),
  float<3>(2.3, 0.25, 2.1),
  float<3>(3.3, 0.25, 2.4),
  float<3>(2.7, 0.25, 2.4),
  float<3>(2.8, 0.0, 2.475),
  float<3>(2.8, -0.25, 2.475),
  float<3>(3.525, -0.25, 2.49375),
  float<3>(3.525, 0.0, 2.49375),
  float<3>(2.9, 0.0, 2.475),
  float<3>(2.9, -0.15, 2.475),
  float<3>(3.45, -0.15, 2.5125),
  float<3>(3.45, 0.0, 2.5125),
  float<3>(2.8, 0.0, 2.4),
  float<3>(2.8, -0.15, 2.4),
  float<3>(3.2, -0.15, 2.4),
  float<3>(3.2, 0.0, 2.4),
  float<3>(3.525, 0.25, 2.49375),
  float<3>(2.8, 0.25, 2.475),
  float<3>(3.45, 0.15, 2.5125),
  float<3>(2.9, 0.15, 2.475),
  float<3>(3.2, 0.15, 2.4),
  float<3>(2.8, 0.15, 2.4),
  float<3>(0.0, 0.0, 3.15),
  float<3>(0.8, 0.0, 3.15),
  float<3>(0.8, -0.45, 3.15),
  float<3>(0.45, -0.8, 3.15),
  float<3>(0.0, -0.8, 3.15),
  float<3>(0.0, 0.0, 2.85),
  float<3>(0.2, 0.0, 2.7),
  float<3>(0.2, -0.112, 2.7),
  float<3>(0.112, -0.2, 2.7),
  float<3>(0.0, -0.2, 2.7),
  float<3>(-0.45, -0.8, 3.15),
  float<3>(-0.8, -0.45, 3.15),
  float<3>(-0.8, 0.0, 3.15),
  float<3>(-0.112, -0.2, 2.7),
  float<3>(-0.2, -0.112, 2.7),
  float<3>(-0.2, 0.0, 2.7),
  float<3>(-0.8, 0.45, 3.15),
  float<3>(-0.45, 0.8, 3.15),
  float<3>(0.0, 0.8, 3.15),
  float<3>(-0.2, 0.112, 2.7),
  float<3>(-0.112, 0.2, 2.7),
  float<3>(0.0, 0.2, 2.7),
  float<3>(0.45, 0.8, 3.15),
  float<3>(0.8, 0.45, 3.15),
  float<3>(0.112, 0.2, 2.7),
  float<3>(0.2, 0.112, 2.7),
  float<3>(0.4, 0.0, 2.55),
  float<3>(0.4, -0.224, 2.55),
  float<3>(0.224, -0.4, 2.55),
  float<3>(0.0, -0.4, 2.55),
  float<3>(1.3, 0.0, 2.55),
  float<3>(1.3, -0.728, 2.55),
  float<3>(0.728, -1.3, 2.55),
  float<3>(0.0, -1.3, 2.55),
  float<3>(1.3, 0.0, 2.4),
  float<3>(1.3, -0.728, 2.4),
  float<3>(0.728, -1.3, 2.4),
  float<3>(0.0, -1.3, 2.4),
  float<3>(-0.224, -0.4, 2.55),
  float<3>(-0.4, -0.224, 2.55),
  float<3>(-0.4, 0.0, 2.55),
  float<3>(-0.728, -1.3, 2.55),
  float<3>(-1.3, -0.728, 2.55),
  float<3>(-1.3, 0.0, 2.55),
  float<3>(-0.728, -1.3, 2.4),
  float<3>(-1.3, -0.728, 2.4),
  float<3>(-1.3, 0.0, 2.4),
  float<3>(-0.4, 0.224, 2.55),
  float<3>(-0.224, 0.4, 2.55),
  float<3>(0.0, 0.4, 2.55),
  float<3>(-1.3, 0.728, 2.55),
  float<3>(-0.728, 1.3, 2.55),
  float<3>(0.0, 1.3, 2.55),
  float<3>(-1.3, 0.728, 2.4),
  float<3>(-0.728, 1.3, 2.4),
  float<3>(0.0, 1.3, 2.4),
  float<3>(0.224, 0.4, 2.55),
  float<3>(0.4, 0.224, 2.55),
  float<3>(0.728, 1.3, 2.55),
  float<3>(1.3, 0.728, 2.55),
  float<3>(0.728, 1.3, 2.4),
  float<3>(1.3, 0.728, 2.4),
  float<3>(0.0, 0.0, 0.0),
  float<3>(1.425, 0.0, 0.0),
  float<3>(1.425, 0.798, 0.0),
  float<3>(0.798, 1.425, 0.0),
  float<3>(0.0, 1.425, 0.0),
  float<3>(1.5, 0.0, 0.075),
  float<3>(1.5, 0.84, 0.075),
  float<3>(0.84, 1.5, 0.075),
  float<3>(0.0, 1.5, 0.075),
  float<3>(-0.798, 1.425, 0.0),
  float<3>(-1.425, 0.798, 0.0),
  float<3>(-1.425, 0.0, 0.0),
  float<3>(-0.84, 1.5, 0.075),
  float<3>(-1.5, 0.84, 0.075),
  float<3>(-1.5, 0.0, 0.075),
  float<3>(-1.425, -0.798, 0.0),
  float<3>(-0.798, -1.425, 0.0),
  float<3>(0.0, -1.425, 0.0),
  float<3>(-1.5, -0.84, 0.075),
  float<3>(-0.84, -1.5, 0.075),
  float<3>(0.0, -1.5, 0.075),
  float<3>(0.798, -1.425, 0.0),
  float<3>(1.425, -0.798, 0.0),
  float<3>(0.84, -1.5, 0.075),
  float<3>(1.5, -0.84, 0.075)
);

auto teapotIndices = uint[512](
  // rim
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
    3,  16,  17,  18,   7,  19,  20,  21,  11,  22,  23,  24,  15,  25,  26,  27,
   18,  28,  29,  30,  21,  31,  32,  33,  24,  34,  35,  36,  27,  37,  38,  39,
   30,  40,  41,   0,  33,  42,  43,   4,  36,  44,  45,   8,  39,  46,  47,  12,

  // body
   12,  13,  14,  15,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
   15,  25,  26,  27,  51,  60,  61,  62,  55,  63,  64,  65,  59,  66,  67,  68,
   27,  37,  38,  39,  62,  69,  70,  71,  65,  72,  73,  74,  68,  75,  76,  77,
   39,  46,  47,  12,  71,  78,  79,  48,  74,  80,  81,  52,  77,  82,  83,  56,
   56,  57,  58,  59,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
   59,  66,  67,  68,  87,  96,  97,  98,  91,  99, 100, 101,  95, 102, 103, 104,
   68,  75,  76,  77,  98, 105, 106, 107, 101, 108, 109, 110, 104, 111, 112, 113,
   77,  82,  83,  56, 107, 114, 115,  84, 110, 116, 117,  88, 113, 118, 119,  92,

  // handle
  120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
  123, 136, 137, 120, 127, 138, 139, 124, 131, 140, 141, 128, 135, 142, 143, 132,
  132, 133, 134, 135, 144, 145, 146, 147, 148, 149, 150, 151,  68, 152, 153, 154,
  135, 142, 143, 132, 147, 155, 156, 144, 151, 157, 158, 148, 154, 159, 160,  68,

  // spout
  161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176,
  164, 177, 178, 161, 168, 179, 180, 165, 172, 181, 182, 169, 176, 183, 184, 173,
  173, 174, 175, 176, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196,
  176, 183, 184, 173, 188, 197, 198, 185, 192, 199, 200, 189, 196, 201, 202, 193,

  // lid
  203, 203, 203, 203, 204, 205, 206, 207, 208, 208, 208, 208, 209, 210, 211, 212,
  203, 203, 203, 203, 207, 213, 214, 215, 208, 208, 208, 208, 212, 216, 217, 218,
  203, 203, 203, 203, 215, 219, 220, 221, 208, 208, 208, 208, 218, 222, 223, 224,
  203, 203, 203, 203, 221, 225, 226, 204, 208, 208, 208, 208, 224, 227, 228, 209,
  209, 210, 211, 212, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
  212, 216, 217, 218, 232, 241, 242, 243, 236, 244, 245, 246, 240, 247, 248, 249,
  218, 222, 223, 224, 243, 250, 251, 252, 246, 253, 254, 255, 249, 256, 257, 258,
  224, 227, 228, 209, 252, 259, 260, 229, 255, 261, 262, 233, 258, 263, 264, 237,

  // bottom
  265, 265, 265, 265, 266, 267, 268, 269, 270, 271, 272, 273,  92, 119, 118, 113,
  265, 265, 265, 265, 269, 274, 275, 276, 273, 277, 278, 279, 113, 112, 111, 104,
  265, 265, 265, 265, 276, 280, 281, 282, 279, 283, 284, 285, 104, 103, 102,  95,
  265, 265, 265, 265, 282, 286, 287, 266, 285, 288, 289, 270,  95,  94,  93,  92
  );

auto cubeVB = new vertex Buffer<float<3>[]>(device, cubeVerts.length);
cubeVB.SetData(&cubeVerts);
auto cubeIB = new index Buffer<uint[]>(device, cubeIndices.length);
cubeIB.SetData(&cubeIndices);

class Tessellator {
  Tessellator(float<3>[]^ controlPoints, uint[]^ controlIndices, int level) {
    int numPatches = controlIndices.length / 16;
    int patchWidth = level + 1;  // FIXME: use wraparound and remove "+ 1"
    int verticesPerPatch = patchWidth * patchWidth;
    int numVertices = numPatches * verticesPerPatch;
    int numIndices = numPatches * level * level * 6;
    vertices = new Vertex[numVertices];
    indices = new uint[numIndices];
    int vi = 0, ii = 0;
    float scale = 1.0 / (float) level;
    for (int k = 0; k < controlIndices.length;) {
      Cubic<float<3>>[4] cubics;
      for (int i = 0; i < 4; ++i) {
        float<3>[4] p;
        for (int j = 0; j < 4; ++j) {
          p[j] = controlPoints[controlIndices[k++]];
        }
        cubics[i].FromBezier(p[0], p[1], p[2], p[3]);
      }
      for (int i = 0; i <= level; ++i) {
        float t = (float) i * scale;
        float<3>[4] p;
        for (int j = 0; j < 4; ++j) {
          p[j] = cubics[j].Evaluate(t);
        }
        Cubic<float<3>> cubic;
        cubic.FromBezier(p[0], p[1], p[2], p[3]);
        for (int j = 0; j <= level; ++j) {
          float s = (float) j * scale;
          vertices[vi].position = cubic.Evaluate(s);
          vertices[vi].normal = cubic.EvaluateTangent(s);
          if (i < level && j < level) {
            indices[ii] = vi;
            indices[ii + 1] = vi + 1;
            indices[ii + 2] = vi + patchWidth + 1;
            indices[ii + 3] = vi;
            indices[ii + 4] = vi + patchWidth + 1;
            indices[ii + 5] = vi + patchWidth;
            ii += 6;
          }
          ++vi;
        }
      }
    }
  }
  Vertex[]* vertices;
  uint[]* indices;
}

Tessellator* tessTeapot = new Tessellator(&teapotControlPoints, &teapotIndices, 8);

auto teapotVB = new vertex Buffer<Vertex[]>(device, tessTeapot.vertices.length);
teapotVB.SetData(tessTeapot.vertices);
auto teapotIB = new index Buffer<uint[]>(device, tessTeapot.indices.length);
teapotIB.SetData(tessTeapot.indices);

class Uniforms {
  float<4,4>  model, view, projection;
//  float<4,4>  viewInverse;
}

class Bindings {
  Sampler* sampler;
  sampled TextureCubeView<float>* textureView;
  uniform Buffer<Uniforms>* uniforms;
}

class SkyboxPipeline {
    float<3> vertexShader(VertexBuiltins vb, float<3> v) vertex {
        auto uniforms = bindings.uniforms.MapReadUniform();
        auto pos = float<4>(v.x, v.y, v.z, 1.0);
        vb.position = uniforms.projection * uniforms.view * uniforms.model * pos;
        return v;
    }
    float<4> fragmentShader(FragmentBuiltins fb, float<3> position) fragment {
      float<3> p = Math.normalize(position);
      // TODO: figure out why the skybox is X-flipped
      return bindings.textureView.Sample(bindings.sampler, float<3>(-p.x, p.y, p.z));
    }
    Bindings bindings;
};

class ReflectionPipeline {
    Vertex vertexShader(VertexBuiltins vb, Vertex v) vertex {
        auto n = Math.normalize(v.normal);
        auto uniforms = bindings.uniforms.MapReadUniform();
        auto viewModel = uniforms.view * uniforms.model;
        auto pos = viewModel * float<4>(v.position.x, v.position.y, v.position.z, 1.0);
        auto normal = viewModel * float<4>(n.x, n.y, n.z, 0.0);
        vb.position = uniforms.projection * pos;
        Vertex varyings;
        varyings.position = float<3>(pos.x, pos.y, pos.z);
        varyings.normal = float<3>(normal.x, normal.y, normal.z);
        return varyings;
    }
    float<4> fragmentShader(FragmentBuiltins fb, Vertex varyings) fragment {
      auto uniforms = bindings.uniforms.MapReadUniform();
      float<3> p = Math.normalize(varyings.position);
      float<3> n = Math.normalize(varyings.normal);
      float<3> r = Math.reflect(-p, n);
      auto r4 = Math.inverse(uniforms.view) * float<4>(r.x, r.y, r.z, 0.0);
      return bindings.textureView.Sample(bindings.sampler, float<3>(-r4.x, r4.y, r4.z));
    }
    Bindings bindings;
};

auto depthState = new DepthStencilState<Depth24Plus>();

auto cubePipeline = new RenderPipeline<SkyboxPipeline>(device, depthState, TriangleList);
auto cubeBindings = new Bindings();
cubeBindings.uniforms = new uniform Buffer<Uniforms>(device);
cubeBindings.sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);
cubeBindings.textureView = texture.CreateSampledCubeView();
auto cubeBindGroup = new BindGroup(device, cubeBindings);

auto teapotPipeline = new RenderPipeline<ReflectionPipeline>(device, depthState, TriangleList);
auto teapotBindings = new Bindings();
teapotBindings.sampler = cubeBindings.sampler;
teapotBindings.textureView = cubeBindings.textureView;
teapotBindings.uniforms = new uniform Buffer<Uniforms>(device);
auto teapotBindGroup = new BindGroup(device, teapotBindings);

float theta = 0.0;
float phi = 0.0;
float distance = 10.0;
int<2> anchor;
float<4, 4> projection = Transform.projection(0.5, 200.0, -1.0, 1.0, -1.0, 1.0);
auto teapotQuat = Quaternion(float<3>(1.0, 0.0, 0.0), -3.1415926 / 2.0);
teapotQuat.normalize();
auto teapotRotation = teapotQuat.toMatrix();
auto depthBuffer = new renderable Texture2D<Depth24Plus>(device, 1024, 1024, 1);
bool mouseIsDown = false;
while (System.IsRunning()) {
  Event* event = System.GetNextEvent();
  if (event.type == MouseDown) {
    mouseIsDown = true;
  } else if (event.type == MouseUp) {
    mouseIsDown = false;
  } else if (event.type == MouseMove) {
    int<2> diff = event.position - anchor;
    if (mouseIsDown || (event.modifiers & Control) != 0) {
      theta += (float) diff.x / 200.0;
      phi += (float) diff.y / 200.0;
    } else if ((event.modifiers & Shift) != 0) {
      distance += (float) diff.y / 100.0;
    }
    anchor = event.position;
  }
  Quaternion orientation = Quaternion(float<3>(0.0, 1.0, 0.0), theta);
  orientation = orientation.mul(Quaternion(float<3>(1.0, 0.0, 0.0), phi));
  orientation.normalize();
  Uniforms uniforms;
  uniforms.projection = projection;
  uniforms.view = Transform.translate(0.0, 0.0, -distance);
  uniforms.view *= orientation.toMatrix();
  uniforms.model = Transform.scale(100.0, 100.0, 100.0);
//  uniforms.viewInverse = Transform.invert(uniforms.view);
  cubeBindings.uniforms.SetData(&uniforms);
  uniforms.model = teapotRotation * Transform.scale(2.0, 2.0, 2.0);
  teapotBindings.uniforms.SetData(&uniforms);
  renderable Texture2DView* framebuffer = swapChain.GetCurrentTextureView();
  CommandEncoder* encoder = new CommandEncoder(device);
  RenderPassEncoder* passEncoder = encoder.BeginRenderPass(framebuffer, depthBuffer.CreateRenderableView());

  passEncoder.SetPipeline(cubePipeline);
  passEncoder.SetBindGroup(0, cubeBindGroup);
  passEncoder.SetVertexBuffer(0, cubeVB);
  passEncoder.SetIndexBuffer(cubeIB);
  passEncoder.DrawIndexed(cubeIndices.length, 1, 0, 0, 0);

  passEncoder.SetPipeline(teapotPipeline);
  passEncoder.SetBindGroup(0, teapotBindGroup);
  passEncoder.SetVertexBuffer(0, teapotVB);
  passEncoder.SetIndexBuffer(teapotIB);
  passEncoder.DrawIndexed(tessTeapot.indices.length, 1, 0, 0, 0);

  passEncoder.End();
  CommandBuffer* cb = encoder.Finish();
  device.GetQueue().Submit(cb);
  swapChain.Present();
}
return 0.0;
