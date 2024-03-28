using Vector = float<2>;
class Utils;

uint width =  10;
uint height = 10;
uint depth =   1;

class Body {
  void computeAcceleration() {
    acceleration = force / mass;
  }

  void applyForce(Vector deltaForce) {
    force += deltaForce;
  }
  Vector   position;
  Vector   velocity;
  Vector   force;
  Vector   acceleration;
  uint[6]  spring;
  float[6] springWeight;
  float    mass;
  float    nailed;
}

class Spring {
  Spring(uint b1, uint b2) {
    body1 = b1;
    body2 = b2;
    ks = 1.0;
    kd = 0.4;
    r = 0.0;
  }
  Vector computeForce(Body b1, Body b2) {
    Vector dp = b1.position - b2.position;
    Vector dv = b1.velocity - b2.velocity;
    float dplen = Utils.length(dp);
    float f = ks * (dplen - r) + kd * Utils.dot(dv, dp) / dplen;
    return -dp / dplen * f;
  }
  uint   body1;
  uint   body2;
  Vector force;
  float  ks;
  float  kd;
  float  r;
  float  placeholder;
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

class Uniforms {
  Vector   gravity;
  Vector   wind;
  Vector   particleSize;
  float    deltaT;
}

class ComputeBindings {
  storage Buffer<Body[]>* bodyStorage;
  storage Buffer<Spring[]>* springStorage;
  storage Buffer<Vector[]>* bodyVerts;
  storage Buffer<Vector[]>* springVerts;
  uniform Buffer<Uniforms>* uniforms;
}

class ComputeBase {
  BindGroup<ComputeBindings>* bindings;
}

class ComputeForces : ComputeBase {
  void computeShader(ComputeBuiltins cb) compute(1, 1, 1) {
    float placeholder1 = Utils.dot(Vector(0.0), Vector(0.0));
    float placeholder2 = Utils.length(Vector(0.0));
    auto bodies = bindings.Get().bodyStorage.MapReadWriteStorage();
    auto springs = bindings.Get().springStorage.MapReadWriteStorage();
    auto u = bindings.Get().uniforms.MapReadUniform();
    uint i = cb.globalInvocationId.x;
    Spring spring = springs[i];
    Body b1 = bodies[spring.body1];
    Body b2 = bodies[spring.body2];
    springs[i].force = spring.computeForce(b1, b2);
  }
}

class ApplyForces : ComputeBase {
  void computeShader(ComputeBuiltins cb) compute(1, 1, 1) {
    auto bodies = bindings.Get().bodyStorage.MapReadWriteStorage();
    auto springs = bindings.Get().springStorage.MapReadWriteStorage();
    auto u = bindings.Get().uniforms.MapReadUniform();
    uint<3> pos = cb.globalInvocationId;
    uint i = cb.globalInvocationId.x;
    Body body = bodies[i];
    Vector force = u.gravity + u.wind;
    for (int i = 0; i < 6; ++i) {
      force += springs[body.spring[i]].force * body.springWeight[i];
    }
    bodies[i].force = force;
  }
}

class FinalizeBodies : ComputeBase {
  void computeShader(ComputeBuiltins cb) compute(1, 1, 1) {
    auto bodies = bindings.Get().bodyStorage.MapReadWriteStorage();
    auto u = bindings.Get().uniforms.MapReadUniform();
    float deltaT = u.deltaT;
    uint i = cb.globalInvocationId.x;
    Body body = bodies[i];
    body.computeAcceleration();
    body.position += body.velocity * deltaT * (1.0 - body.nailed);
    body.velocity += body.acceleration * deltaT * (1.0 - body.nailed);
    auto p = bodies[i].position;
    auto bv = bindings.Get().bodyVerts.MapReadWriteStorage();
    bodies[i] = body;
    bv[i*3]   = p + Vector( u.particleSize.x * 0.5,  0.0);
    bv[i*3+1] = p + Vector(-u.particleSize.x * 0.5,  0.0);
    bv[i*3+2] = p + Vector( 0.0,           -u.particleSize.y);
  }
}

class FinalizeSprings : ComputeBase {
  void computeShader(ComputeBuiltins cb) compute(1, 1, 1) {
    auto bodies = bindings.Get().bodyStorage.MapReadWriteStorage();
    auto springs = bindings.Get().springStorage.MapReadWriteStorage();
    auto sv = bindings.Get().springVerts.MapReadWriteStorage();
    uint i = cb.globalInvocationId.x;
    sv[i*2] = bodies[springs[i].body1].position;
    sv[i*2+1] = bodies[springs[i].body2].position;
  }
}

Device* device = new Device();
Window* window = new Window(device, 0, 0, 960, 960);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);
auto bodies = new Body[width * height * depth];
auto springs = new Spring[bodies.length * 3 - width * depth - height * depth - width * height];
Vector count = Utils.makeVector((float) width, (float) height, (float) depth, Vector(0.0));
Vector pSpacing = Vector(2.0) / count;
int spring = 0;

for (uint i = 0; i < bodies.length; ++i) {
  bodies[i].springWeight[0] = 0.0;
  bodies[i].springWeight[1] = 0.0;
  bodies[i].springWeight[2] = 0.0;
  bodies[i].springWeight[3] = 0.0;
  bodies[i].springWeight[4] = 0.0;
  bodies[i].springWeight[5] = 0.0;
}

for (uint i = 0; i < bodies.length; ++i) {
  uint x = i % width;
  uint y = i % (width * height) / width;
  uint z = i / (width * height);
  Vector pos = Utils.makeVector((float) x, (float) y, (float) z, Vector(0.0));
  bodies[i].position = Vector(-1.0) + pSpacing * (pos + Vector(0.5));
  bodies[i].mass = Math.rand() * 0.5 + 0.25;
  bodies[i].velocity = Vector(0.0);
  bodies[i].acceleration = Vector(0.0);
  bodies[i].nailed = 0.0;

  if (y == 0) {
    if (x == 0) {
      bodies[i].nailed = 1.0;
    }
    if (x == width - 1) {
      bodies[i].nailed = 1.0;
    }
  }
  if (y == width - 1) {
    if (x == 0) {
      bodies[i].nailed = 1.0;
    }
    if (x == width - 1) {
      bodies[i].nailed = 1.0;
    }
  }

  uint body1 = i;
  uint body2 = i + 1;
  if (x < width - 1) {
    springs[spring] = Spring(body1, body2);
    bodies[body1].spring[0] = spring;
    bodies[body2].spring[3] = spring;
    bodies[body1].springWeight[0] = 1.0;
    bodies[body2].springWeight[3] = -1.0;
    ++spring;
  }

  body2 = i + width;
  if (y < height - 1) {
    springs[spring] = Spring(body1, body2);
    bodies[body1].spring[1] = spring;
    bodies[body2].spring[4] = spring;
    bodies[body1].springWeight[1] = 1.0;
    bodies[body2].springWeight[4] = -1.0;
    ++spring;
  }

  body2 = i + width * height;
  if (z < depth - 1) {
    springs[spring] = Spring(body1, body2);
    bodies[body1].spring[2] = spring;
    bodies[body2].spring[5] = spring;
    bodies[body1].springWeight[2] = 1.0;
    bodies[body2].springWeight[5] = -1.0;
    ++spring;
  }
}


int numBodyVerts = bodies.length * 3;
auto bodyVBO = new vertex storage Buffer<Vector[]>(device, numBodyVerts);
int numSpringVerts = springs.length * 2;
auto springVBO = new vertex storage Buffer<Vector[]>(device, numSpringVerts);

auto computeUBO = new uniform Buffer<Uniforms>(device);

auto computeBindGroup = new BindGroup<ComputeBindings>(device, {
  bodyVerts = bodyVBO,
  springVerts = springVBO,
  uniforms = computeUBO,
  bodyStorage = new storage Buffer<Body[]>(device, bodies),
  springStorage = new storage Buffer<Spring[]>(device, springs)
});
class Shaders {
  vertex Buffer<Vector[]>* vert;
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
}

class BodyShaders : Shaders {
  void vertexShader(VertexBuiltins vb) vertex { vb.position = Utils.makeFloat4(vert.Get()); }
  void fragmentShader(FragmentBuiltins fb) fragment { fragColor.Set(float<4>(0.0, 1.0, 0.0, 1.0)); }
}

class SpringShaders : Shaders {
  void vertexShader(VertexBuiltins vb) vertex { vb.position = Utils.makeFloat4(vert.Get()); }
  void fragmentShader(FragmentBuiltins fb) fragment { fragColor.Set(float<4>(1.0, 1.0, 1.0, 1.0)); }
}

auto bodyPipeline = new RenderPipeline<BodyShaders>(device, null, TriangleList);
auto springPipeline = new RenderPipeline<SpringShaders>(device, null, LineList);
auto computeForces = new ComputePipeline<ComputeForces>(device);
auto applyForces = new ComputePipeline<ApplyForces>(device);
auto finalizeBodies = new ComputePipeline<FinalizeBodies>(device);
auto finalizeSprings = new ComputePipeline<FinalizeSprings>(device);
float frequency = 240.0; // physics sim at 240Hz
System.GetNextEvent();
Uniforms* u = new Uniforms();
u.deltaT = 1.0 / frequency;
u.gravity = Vector(0.0, 0.0);
u.particleSize = Vector(0.5) / (float) width;
while(System.IsRunning()) {
  u.wind = Vector(Math.rand() * 0.00, 0.0);
  computeUBO.SetData(u);

  auto framebuffer = swapChain.GetCurrentTexture();
  auto encoder = new CommandEncoder(device);

  auto computePass = new ComputePass<ComputeBase>(encoder, {bindings = computeBindGroup} );

  computePass.SetPipeline(computeForces);
  computePass.Dispatch(springs.length, 1, 1);

  computePass.SetPipeline(applyForces);
  computePass.Dispatch(bodies.length, 1, 1);

  computePass.SetPipeline(finalizeBodies);
  computePass.Dispatch(bodies.length, 1, 1);

  computePass.SetPipeline(finalizeSprings);
  computePass.Dispatch(springs.length, 1, 1);

  computePass.End();
  auto colorAttachment = new ColorAttachment<PreferredSwapChainFormat>(framebuffer, Clear, Store);
  auto renderPass = new RenderPass<Shaders>(encoder, { fragColor = colorAttachment });

  auto bodyPass = new RenderPass<BodyShaders>(renderPass);
  bodyPass.SetPipeline(bodyPipeline);
  bodyPass.Set({vert = bodyVBO});
  bodyPass.Draw(numBodyVerts, 1, 0, 0);

  auto springPass = new RenderPass<SpringShaders>(renderPass);
  springPass.SetPipeline(springPipeline);
  springPass.Set({vert = springVBO});
  springPass.Draw(numSpringVerts, 1, 0, 0);

  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();
}
return 0.0;
