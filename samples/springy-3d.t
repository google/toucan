using Vector = float<3>;
class Utils;

int width =  20;
int height = 20;
int depth =  20;

class Body {
  Vector   position;
  Vector   velocity;
  Vector   force;
  Vector   acceleration;
  float    mass;
  float    movable;

  void computeAcceleration() {
    acceleration = force / mass;
  }

  void eulerStep(float deltaT) {
    position += velocity * deltaT * movable;
    velocity += acceleration * deltaT * movable;
  }
}

class Spring {
  int    body1;
  int    body2;
  float  ks;
  float  kd;
  float  r;

  Vector computeForce(Body b1, Body b2) {
    Vector dp = b1.position - b2.position;
    Vector dv = b1.velocity - b2.velocity;
    float dplen = Utils.length(dp);
    float f = ks * (dplen - r) + kd * Utils.dot(dv, dp) / dplen;
    return -dp / dplen * f;
  }

  Spring(int b1, int b2) {
    body1 = b1;
    body2 = b2;
    ks = 3.0;
    kd = 0.3;
    r = 0.0;
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
  Quaternion conj() { return Quaternion(-q.x, -q.y, -q.z, q.w); }
  Quaternion mul(float f) { return Quaternion(q * f); }
  Quaternion mul(Quaternion other) {
    float<4> p = other.q;
    Quaternion r;

    r.q.x = p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y;
    r.q.y = p.w * q.y + p.y * q.w + p.z * q.x - p.x * q.z;
    r.q.z = p.w * q.z + p.z * q.w + p.x * q.y - p.y * q.x;
    r.q.w = p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z;

    return r;
  }
  float<3> mul(float<3> v) {
    auto s = Quaternion(v.x, v.y, v.z, 0.0);
    float<4> r = this.conj().mul(s).mul(Quaternion(q)).q;
    return float<3>(q.x, q.y, q.z);
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

class Utils {
  static float dot(float<2> v1, float<2> v2) {
    float<2> r = v1 * v2;
    return r.x + r.y;
  }
  static float dot(float<3> v1, float<3> v2) {
    float<3> r = v1 * v2;
    return r.x + r.y + r.z;
  }
  static float dot(float<4> v1, float<4> v2) {
    float<4> r = v1 * v2;
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
}

class ParticleSystem {
  Body[]* bodies;
  Spring[]* springs;
  ParticleSystem(Body[]* b, Spring[]* s) {
    bodies = b;
    springs = s;
  }

  void eulerStep(float deltaT) {
    Vector gravity = Vector(0.0, -0.25);
    Vector wind = Vector(Math.rand() * 0.05, 0.0);
    for (int i = 0; i < bodies.length; ++i) {
      bodies[i].force = gravity + wind;
    }
    for (int i = 0; i < springs.length; ++i) {
      int b1 = springs[i].body1;
      int b2 = springs[i].body2;
      Vector fk = springs[i].computeForce(bodies[b1], bodies[b2]);
      bodies[b1].force += fk;
      bodies[b2].force -= fk;
    }
    for (int i = 0; i < bodies.length; ++i) {
      bodies[i].computeAcceleration();
      bodies[i].eulerStep(deltaT);
    }
  }
}

class DrawUniforms {
  float<4,4>  matrix;
  float<4>    color;
}

class DrawPipeline {
  void vertexShader(VertexBuiltins vb, Vector v) vertex {
    auto matrix = uniforms.MapReadUniform().matrix;
    vb.position = matrix * Utils.makeFloat4(v);
  }
  float<4> fragmentShader(FragmentBuiltins fb) fragment {
    return uniforms.MapReadUniform().color;
  }
  uniform Buffer<DrawUniforms>* uniforms;
}

auto bodies = new Body[width * height * depth];
auto springs = new Spring[bodies.length * 3 - width * depth - height * depth - width * height];
int spring = 0;
for (int i = 0; i < bodies.length; ++i) {
  int x = i % width;
  int y = i % (width * height) / width;
  int z = i / (width * height);
  Vector pos = Utils.makeVector((float) (x - width / 2) + 0.5,
                                (float) (y - height / 2) + 0.5,
                                (float) (z - depth / 2) + 0.5, Vector(0.0));
  bodies[i].position = pos;
  bodies[i].mass = Math.rand() * 2.5 + 1.25;
  bodies[i].velocity = Vector(0.0);
  bodies[i].acceleration = Vector(0.0);
  if (y == height - 1) {
    bodies[i].movable = 0.0;
  } else {
    bodies[i].movable = 1.0;
  }

  if (x < width - 1) {
    springs[spring++] = Spring(i, i + 1);
  }

  if (y < height - 1) {
    springs[spring++] = Spring(i, i + width);
  }

  if (z < depth - 1) {
    springs[spring++] = Spring(i, i + width * height);
  }
}

Device* device = new Device();
Window* window = new Window(device, 0, 0, 960, 960);
SwapChain* swapChain = new SwapChain(window);
auto physicsSystem = new ParticleSystem(bodies, springs);

auto bodyVerts = new Vector[bodies.length * 3];
auto bodyVBO = new vertex Buffer<Vector[]>(device, bodyVerts.length);

auto springVerts = new Vector[springs.length * 2];
auto springVBO = new vertex Buffer<Vector[]>(device, springVerts.length);

auto bodyPipeline = new RenderPipeline<DrawPipeline>(device, null, TriangleList);
auto springPipeline = new RenderPipeline<DrawPipeline>(device, null, LineList);
auto bodyUBO = new uniform Buffer<DrawUniforms>(device);
auto bodyBG = new BindGroup(device, bodyUBO);
auto springUBO = new uniform Buffer<DrawUniforms>(device);
auto springBG = new BindGroup(device, springUBO);
float theta = 0.0;
float phi = 0.0;
float distance = 0.5 * (float) width;
auto drawUniforms = new DrawUniforms();
float<4,4> projection = Utils.projection(1.0, 100.0, -1.0, 1.0, -1.0, 1.0);
int<2> anchor;
double startTime = System.GetCurrentTime();
float frequency = 480.0;
int maxStepsPerFrame = 32;
int stepsDone = 0;
while(System.IsRunning()) {
  while (System.HasPendingEvents()) {
    Event* event = System.GetNextEvent();
    if (event.type == MouseMove) {
      int<2> diff = event.position - anchor;
      if ((event.modifiers & Control) != 0) {
        theta += (float) diff.x / 200.0;
        phi += (float) diff.y / 200.0;
      } else if ((event.modifiers & Shift) != 0) {
        distance += (float) diff.y / 100.0;
      }
      anchor = event.position;
    }
  }
  Quaternion orientation = Quaternion(float<3>(0.0, 1.0, 0.0), theta);
  orientation = orientation.mul(Quaternion(float<3>(1.0, 0.0, 0.0), phi));
  orientation.normalize();
  drawUniforms.matrix = projection;
  drawUniforms.matrix *= Utils.translate(0.0, 0.0, -distance);
  drawUniforms.matrix *= orientation.toMatrix();
  drawUniforms.color = float<4>(1.0, 1.0, 1.0, 1.0);
  springUBO.SetData(drawUniforms);
  drawUniforms.color = float<4>(0.0, 1.0, 0.0, 1.0);
  bodyUBO.SetData(drawUniforms);
  for (int i = 0; i < bodies.length; ++i) {
    auto p = bodies[i].position;
    bodyVerts[i*3  ] = p + Vector( 0.1,  0.0);
    bodyVerts[i*3+1] = p + Vector(-0.1,  0.0);
    bodyVerts[i*3+2] = p + Vector( 0.0, -0.2);
  }
  bodyVBO.SetData(bodyVerts);

  for (int i = 0; i < springs.length; ++i) {
    springVerts[i*2] = bodies[springs[i].body1].position;
    springVerts[i*2+1] = bodies[springs[i].body2].position;
  }
  springVBO.SetData(springVerts);

  int totalSteps = (int) ((System.GetCurrentTime() - startTime) * frequency);

  for (int i = 0; stepsDone < totalSteps && i < maxStepsPerFrame; i++) {
    physicsSystem.eulerStep(8.0 / frequency);
    stepsDone++;
  }
  renderable Texture2DView* framebuffer = swapChain.GetCurrentTextureView();
  CommandEncoder* encoder = new CommandEncoder(device);
  RenderPassEncoder* passEncoder = encoder.BeginRenderPass(framebuffer);

  passEncoder.SetPipeline(springPipeline);
  passEncoder.SetVertexBuffer(0, springVBO);
  passEncoder.SetBindGroup(0, springBG);
  passEncoder.Draw(springVerts.length, 1, 0, 0);

  passEncoder.SetPipeline(bodyPipeline);
  passEncoder.SetVertexBuffer(0, bodyVBO);
  passEncoder.SetBindGroup(0, bodyBG);
  passEncoder.Draw(bodyVerts.length, 1, 0, 0);

  passEncoder.End();
  CommandBuffer* cb = encoder.Finish();
  device.GetQueue().Submit(cb);
  swapChain.Present();
}
return 0.0;
