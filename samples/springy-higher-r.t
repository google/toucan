include "event-handler.t"
include "quaternion.t"
include "transform.t"
include "utils.t"

using Vector = float<2>;

int width =  10;
int height = 10;
int depth =   1;

class Body {
  Vector   position;
  Vector   velocity;
  Vector   force;
  Vector   acceleration;
  float    mass;
  float    movable;
  int[6]   spring;
  float[6] springWeight;

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
  Vector force;

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
    ks = 1.0;
    kd = 0.4;
    r = 0.3;
  }
}

class ComputeUniforms {
  Vector       gravity;
  Vector       wind;
  float        deltaT;
}

class DrawUniforms {
  float<4,4>   matrix;
  float<4>     color;
}

class ComputeBindings {
  storage Buffer<Body[]>* bodyStorage;
  storage Buffer<Spring[]>* springStorage;
  storage Buffer<Vector[]>* bodyVerts;
  storage Buffer<Vector[]>* springVerts;
  uniform Buffer<ComputeUniforms>* uniforms;
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
    uint i = cb.globalInvocationId.x;
    Body body = bodies[i];
    body.force = u.gravity + u.wind;
    for (int i = 0; i < 6; ++i) {
      body.force += springs[body.spring[i]].force * body.springWeight[i];
    }
    body.computeAcceleration();
    body.eulerStep(u.deltaT);
    bodies[i] = body;
  }
}

class UpdateBodyVerts : ComputeBase {
  void computeShader(ComputeBuiltins cb) compute(1, 1, 1) {
    auto bodies = bindings.Get().bodyStorage.MapReadWriteStorage();
    uint i = cb.globalInvocationId.x;
    auto p = bodies[i].position;
    auto bv = bindings.Get().bodyVerts.MapReadWriteStorage();
    bv[i*3]   = p + Vector( 0.1,  0.0);
    bv[i*3+1] = p + Vector(-0.1,  0.0);
    bv[i*3+2] = p + Vector( 0.0, -0.2);
  }
}

class UpdateSpringVerts : ComputeBase {
  void computeShader(ComputeBuiltins cb) compute(1, 1, 1) {
    auto bodies = bindings.Get().bodyStorage.MapReadWriteStorage();
    auto springs = bindings.Get().springStorage.MapReadWriteStorage();
    auto sv = bindings.Get().springVerts.MapReadWriteStorage();
    uint i = cb.globalInvocationId.x;
    sv[i*2] = bodies[springs[i].body1].position;
    sv[i*2+1] = bodies[springs[i].body2].position;
  }
}

class DrawBindings {
  uniform Buffer<DrawUniforms>* uniforms;
}

class DrawPipeline {
  void vertexShader(VertexBuiltins vb) vertex {
    auto matrix = bindings.Get().uniforms.MapReadUniform().matrix;
    vb.position = matrix * Utils.makeFloat4(vertices.Get());
  }
  void fragmentShader(FragmentBuiltins fb) fragment {
    fragColor.Set(bindings.Get().uniforms.MapReadUniform().color);
  }
  vertex Buffer<Vector[]>* vertices;
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
  BindGroup<DrawBindings>* bindings;
}

auto bodies = new Body[width * height * depth];
auto springs = new Spring[bodies.length * 3 - width * depth - height * depth - width * height];
int spring = 0;

for (int i = 0; i < bodies.length; ++i) {
  bodies[i].springWeight[0] = 0.0;
  bodies[i].springWeight[1] = 0.0;
  bodies[i].springWeight[2] = 0.0;
  bodies[i].springWeight[3] = 0.0;
  bodies[i].springWeight[4] = 0.0;
  bodies[i].springWeight[5] = 0.0;
}

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

  int body1 = i;
  if (x < width - 1) {
    int body2 = i + 1;
    springs[spring] = Spring(body1, body2);
    bodies[body1].spring[0] = spring;
    bodies[body2].spring[3] = spring;
    bodies[body1].springWeight[0] = 1.0;
    bodies[body2].springWeight[3] = -1.0;
    ++spring;
  }

  if (y < height - 1) {
    int body2 = i + width;
    springs[spring] = Spring(body1, body2);
    bodies[body1].spring[1] = spring;
    bodies[body2].spring[4] = spring;
    bodies[body1].springWeight[1] = 1.0;
    bodies[body2].springWeight[4] = -1.0;
    ++spring;
  }

  if (z < depth - 1) {
    int body2 = i + width * height;
    springs[spring] = Spring(body1, body2);
    bodies[body1].spring[2] = spring;
    bodies[body2].spring[5] = spring;
    bodies[body1].springWeight[2] = 1.0;
    bodies[body2].springWeight[5] = -1.0;
    ++spring;
  }
}

Device* device = new Device();
Window* window = new Window(device, 0, 0, 960, 960);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);

int numBodyVerts = bodies.length * 3;
auto bodyVBO = new vertex storage Buffer<Vector[]>(device, numBodyVerts);
int numSpringVerts = springs.length * 2;
auto springVBO = new vertex storage Buffer<Vector[]>(device, numSpringVerts);

auto computeUBO = new uniform Buffer<ComputeUniforms>(device);

auto computeBindGroup = new BindGroup<ComputeBindings>(device, {
  bodyVerts = bodyVBO,
  springVerts = springVBO,
  uniforms = computeUBO,
  bodyStorage = new storage Buffer<Body[]>(device, bodies),
  springStorage = new storage Buffer<Spring[]>(device, springs)
});

auto bodyPipeline = new RenderPipeline<DrawPipeline>(device, null, TriangleList);
auto springPipeline = new RenderPipeline<DrawPipeline>(device, null, LineList);
auto computeForces = new ComputePipeline<ComputeForces>(device);
auto applyForces = new ComputePipeline<ApplyForces>(device);
auto updateBodyVerts = new ComputePipeline<UpdateBodyVerts>(device);
auto updateSpringVerts = new ComputePipeline<UpdateSpringVerts>(device);
float frequency = 480.0;
int maxStepsPerFrame = 32;
int stepsDone = 0;
auto bodyUBO = new uniform Buffer<DrawUniforms>(device);
DrawBindings bodyBindings;
bodyBindings.uniforms = bodyUBO;
auto bodyBG = new BindGroup<DrawBindings>(device, &bodyBindings);
auto springUBO = new uniform Buffer<DrawUniforms>(device);
DrawBindings springBindings;
springBindings.uniforms = springUBO;
auto springBG = new BindGroup<DrawBindings>(device, &springBindings);
EventHandler handler;
handler.rotation = float<2>(0.0, 0.0);
handler.distance = 0.5 * (float) width;
ComputeUniforms computeUniforms;
float<4,4> projection = Transform.projection(1.0, 100.0, -1.0, 1.0, -1.0, 1.0);
computeUniforms.deltaT = 8.0 / frequency;
computeUniforms.gravity = Vector(0.0, -0.25);
double startTime = System.GetCurrentTime();
while(System.IsRunning()) {
  Quaternion orientation = Quaternion(float<3>(0.0, 1.0, 0.0), handler.rotation.x);
  orientation = orientation.mul(Quaternion(float<3>(1.0, 0.0, 0.0), handler.rotation.y));
  orientation.normalize();
  auto matrix = projection;
  matrix *= Transform.translate(0.0, 0.0, -handler.distance);
  matrix *= orientation.toMatrix();

  springUBO.SetData({matrix = matrix, color = {1.0, 1.0, 1.0, 1.0}});
  bodyUBO.SetData({matrix = matrix, color = {0.0, 1.0, 0.0, 1.0}});

  computeUniforms.wind = Vector(Math.rand() * 0.01, 0.0);
  computeUBO.SetData(&computeUniforms);

  auto encoder = new CommandEncoder(device);
  auto computePass = new ComputePass<ComputeBase>(encoder, {bindings = computeBindGroup});

  int totalSteps = (int) ((System.GetCurrentTime() - startTime) * frequency);
  for (int i = 0; stepsDone < totalSteps && i < maxStepsPerFrame; i++) {
    computePass.SetPipeline(computeForces);
    computePass.Dispatch(springs.length, 1, 1);

    computePass.SetPipeline(applyForces);
    computePass.Dispatch(bodies.length, 1, 1);
    stepsDone++;
  }

  computePass.SetPipeline(updateBodyVerts);
  computePass.Dispatch(bodies.length, 1, 1);

  computePass.SetPipeline(updateSpringVerts);
  computePass.Dispatch(springs.length, 1, 1);

  computePass.End();
  auto framebuffer = swapChain.GetCurrentTexture();
  DrawPipeline p;
  p.fragColor = new ColorAttachment<PreferredSwapChainFormat>(framebuffer, Clear, Store);
  auto renderPass = new RenderPass<DrawPipeline>(encoder, &p);

  renderPass.SetPipeline(springPipeline);
  renderPass.Set({vertices = springVBO, bindings = springBG});
  renderPass.Draw(numSpringVerts, 1, 0, 0);

  renderPass.SetPipeline(bodyPipeline);
  renderPass.Set({vertices = bodyVBO, bindings = bodyBG});
  renderPass.Draw(numBodyVerts, 1, 0, 0);

  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();

  while (System.HasPendingEvents()) {
    handler.Handle(System.GetNextEvent());
  }
}
return 0.0;
