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

auto cubeVB = new vertex Buffer<float<3>[]>(device, cubeVerts.length);
cubeVB.SetData(&cubeVerts);
auto cubeIB = new index Buffer<uint[]>(device, cubeIndices.length);
cubeIB.SetData(&cubeIndices);

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

auto depthState = new DepthStencilState<Depth24Plus>();

auto cubePipeline = new RenderPipeline<SkyboxPipeline>(device, depthState, TriangleList);
auto cubeBindings = new Bindings();
cubeBindings.uniforms = new uniform Buffer<Uniforms>(device, 1);
cubeBindings.sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);
cubeBindings.textureView = texture.CreateSampledCubeView();
auto cubeBindGroup = new BindGroup(device, cubeBindings);

float theta = 0.0;
float phi = 0.0;
float distance = 10.0;
int<2> anchor;
float<4, 4> projection = Transform.projection(0.5, 200.0, -1.0, 1.0, -1.0, 1.0);
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
  renderable Texture2DView* framebuffer = swapChain.GetCurrentTextureView();
  CommandEncoder* encoder = new CommandEncoder(device);
  RenderPassEncoder* passEncoder = encoder.BeginRenderPass(framebuffer, depthBuffer.CreateRenderableView(), 0.0, 0.0, 0.0, 0.0);

  passEncoder.SetPipeline(cubePipeline);
  passEncoder.SetBindGroup(0, cubeBindGroup);
  passEncoder.SetVertexBuffer(0, cubeVB);
  passEncoder.SetIndexBuffer(cubeIB);
  passEncoder.DrawIndexed(cubeIndices.length, 1, 0, 0, 0);

  passEncoder.End();
  CommandBuffer* cb = encoder.Finish();
  device.GetQueue().Submit(cb);
  swapChain.Present();
}
return 0.0;
