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

class Bindings {
  uniform Buffer<DrawUniforms>* uniforms;
}

class DrawPipeline {
  void vertexShader(VertexBuiltins^ vb) vertex {
    var matrix = bindings.Get().uniforms.MapReadUniform().matrix;
    vb.position = matrix * Utils.makeFloat4(vertices.Get());
  }
  void fragmentShader(FragmentBuiltins^ fb) fragment {
    fragColor.Set(bindings.Get().uniforms.MapReadUniform().color);
  }
  vertex Buffer<Vector[]>* vertices;
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
  BindGroup<Bindings>* bindings;
}

var bodies = new Body[width * height * depth];
var springs = new Spring[bodies.length * 3 - width * depth - height * depth - width * height];
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
Window* window = new Window({0, 0}, {960, 960});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var physicsSystem = new ParticleSystem(bodies, springs);

var bodyVerts = new Vector[bodies.length * 3];
var bodyVBO = new vertex Buffer<Vector[]>(device, bodyVerts.length);

var springVerts = new Vector[springs.length * 2];
var springVBO = new vertex Buffer<Vector[]>(device, springVerts.length);

var bodyPipeline = new RenderPipeline<DrawPipeline>(device, null, TriangleList);
var springPipeline = new RenderPipeline<DrawPipeline>(device, null, LineList);
Bindings bodyBindings;
bodyBindings.uniforms = new uniform Buffer<DrawUniforms>(device);
var bodyBG = new BindGroup<Bindings>(device, &bodyBindings);
Bindings springBindings;
springBindings.uniforms = new uniform Buffer<DrawUniforms>(device);
var springBG = new BindGroup<Bindings>(device, &springBindings);
EventHandler handler;
handler.rotation = float<2>(0.0, 0.0);
handler.distance = 0.5 * (float) width;
var drawUniforms = new DrawUniforms();
float<4,4> projection = Transform.projection(1.0, 100.0, -1.0, 1.0, -1.0, 1.0);
double startTime = System.GetCurrentTime();
float frequency = 480.0;
int maxStepsPerFrame = 32;
int stepsDone = 0;
while(System.IsRunning()) {
  Quaternion orientation = Quaternion(float<3>(0.0, 1.0, 0.0), handler.rotation.x);
  orientation = orientation.mul(Quaternion(float<3>(1.0, 0.0, 0.0), handler.rotation.y));
  orientation.normalize();
  drawUniforms.matrix = projection;
  drawUniforms.matrix *= Transform.translate(0.0, 0.0, -handler.distance);
  drawUniforms.matrix *= orientation.toMatrix();
  drawUniforms.color = float<4>(1.0, 1.0, 1.0, 1.0);
  springBindings.uniforms.SetData(drawUniforms);
  drawUniforms.color = float<4>(0.0, 1.0, 0.0, 1.0);
  bodyBindings.uniforms.SetData(drawUniforms);
  for (int i = 0; i < bodies.length; ++i) {
    var p = bodies[i].position;
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
  var framebuffer = swapChain.GetCurrentTexture();
  var encoder = new CommandEncoder(device);
  var colorAttachment = new ColorAttachment<PreferredSwapChainFormat>(framebuffer, Clear, Store);
  var renderPass = new RenderPass<DrawPipeline>(encoder, { fragColor = colorAttachment });

  renderPass.SetPipeline(springPipeline);
  renderPass.Set({vertices = springVBO, bindings = springBG});
  renderPass.Draw(springVerts.length, 1, 0, 0);

  renderPass.SetPipeline(bodyPipeline);
  renderPass.Set({vertices = bodyVBO, bindings = bodyBG});
  renderPass.Draw(bodyVerts.length, 1, 0, 0);

  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();

  while (System.HasPendingEvents()) {
    handler.Handle(System.GetNextEvent());
  }
}
