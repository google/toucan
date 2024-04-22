using Vector = float<2>;
class Utils;

int width =  10;
int height = 10;
int depth =   1;

class Pair<T> {
  T      p;
  T      q;
}

class Body<T> {
  T      position;
  T      velocity;
  T      force;
  float  mass;
  T      acceleration;
  bool   nailed;

  void derivEval() {
    acceleration = force / mass;
  }

  void applyForce(T deltaForce) {
    force += deltaForce;
  }
}

class Spring<T> {
  int   body1;
  int   body2;
  float ks;
  float kd;
  float r;
  void applyForce(Body<T>[]* bodies) {
    T dp = bodies[body1].position - bodies[body2].position;
    T dv = bodies[body1].velocity - bodies[body2].velocity;
    float dplen = Utils.length(dp);
    float f = ks * (dplen - r) + kd * Utils.dot(dv, dp) / dplen;
    T fk = -dp / dplen * f;
    bodies[body1].applyForce(fk);
    bodies[body2].applyForce(-fk);
  }
}

class ParticleSystem<T> {
  ParticleSystem(Body<T>[]* b, Spring<T>[]* s) {
    bodies = b;
    springs = s;
    x = new Pair<T>[bodies.length];
    dx = new Pair<T>[bodies.length];
    x1 = new Pair<T>[bodies.length];
    x2 = new Pair<T>[bodies.length];
    x3 = new Pair<T>[bodies.length];
    k1 = new Pair<T>[bodies.length];
    k2 = new Pair<T>[bodies.length];
    k3 = new Pair<T>[bodies.length];
    k4 = new Pair<T>[bodies.length];
  }

  void getState(Pair<T>[]^ p) {
    for (int i = 0; i < bodies.length; ++i) {
      p[i].p = bodies[i].position;
      p[i].q = bodies[i].velocity;
    }
  }

  void setState(Pair<T>[]^ p) {
    for (int i = 0; i < bodies.length; ++i) {
      if (!bodies[i].nailed) {
        bodies[i].position = p[i].p;
        bodies[i].velocity = p[i].q;
      }
    }
  }

  void eulerStep(float deltaT) {
    this.getState(x);

    this.derivEval(dx);

    for (int i = 0; i < bodies.length; ++i) {
      x[i].p += dx[i].p * deltaT;
      x[i].q += dx[i].q * deltaT;
    }

    this.setState(x);
  }

  void midpointStep(float deltaT) {
    this.getState(x1);
    this.eulerStep(deltaT * 0.5);
    this.derivEval(dx);
    for (int i = 0; i < bodies.length; ++i) {
      x1[i].p += dx[i].p * deltaT;
      x1[i].q += dx[i].q * deltaT;
    }
    this.setState(x1);
  }

  void rungeKutta4Step(float deltaT) {
    this.getState(x);
    this.derivEval(dx);
    for (int i = 0; i < bodies.length; ++i) {
      k1[i].p = dx[i].p * deltaT;
      k1[i].q = dx[i].q * deltaT;
      x1[i].p = x[i].p + k1[i].p * 0.5;
      x1[i].q = x[i].q + k1[i].q * 0.5;
    }
    this.setState(x1);
    this.derivEval(dx);
    for (int i = 0; i < bodies.length; ++i) {
      k2[i].p = dx[i].p * deltaT;
      k2[i].q = dx[i].q * deltaT;
      x2[i].p = x[i].p + k2[i].p * 0.5;
      x2[i].q = x[i].q + k2[i].q * 0.5;
    }
    this.setState(x2);
    this.derivEval(dx);
    for (int i = 0; i < bodies.length; ++i) {
      k3[i].p = dx[i].p * deltaT;
      k3[i].q = dx[i].q * deltaT;
      x3[i].p = x[i].p + k3[i].p;
      x3[i].q = x[i].q + k3[i].q;
    }
    this.setState(x3);
    this.derivEval(dx);
    for (int i = 0; i < bodies.length; ++i) {
      k4[i].p = dx[i].p * deltaT;
      k4[i].q = dx[i].q * deltaT;
      x[i].p += k1[i].p / 6.0 + k2[i].p / 3.0 + k3[i].p / 3.0 + k4[i].p / 6.0;
      x[i].q += k1[i].q / 6.0 + k2[i].q / 3.0 + k3[i].q / 3.0 + k4[i].q / 6.0;
    }
    this.setState(x);
  }

  void derivEval(Pair<T>[]^ result) {
    T gravity = T(0.0, -0.05);
    T wind = T(Math.rand() * 0.01, 0.0);
    for (int i = 0; i < bodies.length; ++i) {
      bodies[i].force = gravity + wind;
    }
    for (int i = 0; i < springs.length; ++i) {
      springs[i].applyForce(bodies);
    }
    for (int i = 0; i < bodies.length; ++i) {
      bodies[i].derivEval();
      result[i].p = bodies[i].velocity;
      result[i].q = bodies[i].acceleration;
    }
  }
  Body<T>[]* bodies;
  Spring<T>[]* springs;
  Pair<T>[]* x;
  Pair<T>[]* dx;
  Pair<T>[]* x1;
  Pair<T>[]* x2;
  Pair<T>[]* x3;
  Pair<T>[]* k1;
  Pair<T>[]* k2;
  Pair<T>[]* k3;
  Pair<T>[]* k4;
}

class Utils {
  static float dot(float<2> v1, float<2> v2) {
    float<2> r = v1 * v2;
    return r.x + r.y;
  }
  static float length(float<2> v) {
    return Math.sqrt(Utils.dot(v, v));
  }
  static float dot(float<3> v1, float<3> v2) {
    float<3> r = v1 * v2;
    return r.x + r.y + r.z;
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
}

Device* device = new Device();
Window* window = new Window(device, 0, 0, 960, 960);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);
auto bodies = new Body<Vector>[width * height * depth];
auto springs = new Spring<Vector>[bodies.length * 3 - width * depth - height * depth - width * height];
Vector count = Utils.makeVector((float) width, (float) height, (float) depth, Vector(0.0));
Vector particleSize = Vector(0.5) / count;
Vector pSpacing = Vector(2.0) / count;
int springIndex = 0;
for (int i = 0; i < bodies.length; ++i) {
  int x = i % width;
  int y = i % (width * height) / width;
  int z = i / (width * height);
  Vector pos = Utils.makeVector((float) x, (float) y, (float) z, Vector(0.0));
  bodies[i].position = Vector(-1.0) + pSpacing * (pos + Vector(0.5));
  bodies[i].mass = Math.rand() * 0.5 + 0.25;
  bodies[i].velocity = Vector(0.0);
  bodies[i].acceleration = Vector(0.0);
  bodies[i].nailed = y == height - 1;

  if (x < width - 1) {
    springs[springIndex].body1 = i;
    springs[springIndex].body2 = i + 1;
    springs[springIndex].ks = 1.0;
    springs[springIndex].kd = 0.0;
    springs[springIndex].r = 0.0;
    ++springIndex;
  }

  if (y < height - 1) {
    springs[springIndex].body1 = i;
    springs[springIndex].body2 = i + width;
    springs[springIndex].ks = 1.0;
    springs[springIndex].kd = 0.0;
    springs[springIndex].r = 0.0;
    ++springIndex;
  }

  if (z < depth - 1) {
    springs[springIndex].body1 = i;
    springs[springIndex].body2 = i + width * height;
    springs[springIndex].ks = 1.0;
    springs[springIndex].kd = 0.0;
    springs[springIndex].r = 0.0;
    ++springIndex;
  }
}

auto placeholder = new Pair<Vector>();

auto physicsSystem = new ParticleSystem<Vector>(bodies, springs);

auto bodyVerts = new Vector[bodies.length * 3];
auto bodyVBO = new vertex Buffer<Vector[]>(device, bodyVerts.length);

auto springVerts = new Vector[springs.length * 2];
auto springVBO = new vertex Buffer<Vector[]>(device, springVerts.length);

class BodyShaders<T> {
  void vertexShader(VertexBuiltins vb, T v) vertex { vb.position = Utils.makeFloat4(v); }
  float<4> fragmentShader(FragmentBuiltins fb) fragment { return float<4>(0.0, 1.0, 0.0, 1.0); }
}

class SpringShaders<T> {
  void vertexShader(VertexBuiltins vb, T v) vertex { vb.position = Utils.makeFloat4(v); }
  float<4> fragmentShader(FragmentBuiltins fb) fragment { return float<4>(1.0, 1.0, 1.0, 1.0); }
}

RenderPipeline* bodyPipeline = new RenderPipeline<BodyShaders<Vector>>(device, null, TriangleList);
RenderPipeline* springPipeline = new RenderPipeline<SpringShaders<Vector>>(device, null, LineList);
float frequency = 240.0; // physics sim at 240Hz
System.GetNextEvent();
while(System.IsRunning()) {
  for (int i = 0; i < bodies.length; ++i) {
    auto p = bodies[i].position;
    bodyVerts[i*3  ] = p + Utils.makeVector( particleSize.x * 0.5,  0.0,     0.0, Vector(0.0));
    bodyVerts[i*3+1] = p + Utils.makeVector(-particleSize.x * 0.5,  0.0,     0.0, Vector(0.0));
    bodyVerts[i*3+2] = p + Utils.makeVector( 0.0,           -particleSize.y, 0.0, Vector(0.0));
  }
  bodyVBO.SetData(bodyVerts);

  for (int i = 0; i < springs.length; ++i) {
    springVerts[i*2] = bodies[springs[i].body1].position;
    springVerts[i*2+1] = bodies[springs[i].body2].position;
  }
  springVBO.SetData(springVerts);

  physicsSystem.rungeKutta4Step(1.0 / frequency);
  auto framebuffer = swapChain.GetCurrentTexture();
  auto encoder = new CommandEncoder(device);
  auto renderPass = new RenderPass(encoder, framebuffer);

  renderPass.SetPipeline(bodyPipeline);
  renderPass.SetVertexBuffer(0, bodyVBO);
  renderPass.Draw(bodyVerts.length, 1, 0, 0);

  renderPass.SetPipeline(springPipeline);
  renderPass.SetVertexBuffer(0, springVBO);
  renderPass.Draw(springVerts.length, 1, 0, 0);

  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();
}
return 0.0;
