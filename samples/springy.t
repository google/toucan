include "event-handler.t"
include "quaternion.t"
include "transform.t"
include "utils.t"

using Vector = float<2>;

var width =  10;
var height = 10;
var depth =   1;

class Body {
  var position : Vector;
  var velocity : Vector;
  var force : Vector;
  var acceleration : Vector;
  var mass : float;
  var movable : float;

  computeAcceleration() {
    acceleration = force / mass;
  }

  eulerStep(float deltaT) {
    position += velocity * deltaT * movable;
    velocity += acceleration * deltaT * movable;
  }
}

class Spring {
  var body1 : int;
  var body2 : int;
  var ks : float;
  var kd : float;
  var r : float;

  computeForce(Body b1, Body b2) : Vector {
    var dp = b1.position - b2.position;
    var dv = b1.velocity - b2.velocity;
    var dplen = Utils.length(dp);
    var f = ks * (dplen - r) + kd * Utils.dot(dv, dp) / dplen;
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
  var bodies : Body[]*;
  var springs : Spring[]*;
  ParticleSystem(Body[]* b, Spring[]* s) {
    bodies = b;
    springs = s;
  }

  eulerStep(float deltaT) {
    var gravity = Vector(0.0, -0.25);
    var wind = Vector(Math.rand() * 0.05, 0.0);
    for (var i = 0; i < bodies.length; ++i) {
      bodies[i].force = gravity + wind;
    }
    for (var i = 0; i < springs.length; ++i) {
      var b1 = springs[i].body1;
      var b2 = springs[i].body2;
      var fk = springs[i].computeForce(bodies[b1], bodies[b2]);
      bodies[b1].force += fk;
      bodies[b2].force -= fk;
    }
    for (var i = 0; i < bodies.length; ++i) {
      bodies[i].computeAcceleration();
      bodies[i].eulerStep(deltaT);
    }
  }
}

class DrawUniforms {
  var matrix : float<4,4>;
  var color : float<4>;
}

class Bindings {
  var uniforms : uniform Buffer<DrawUniforms>*;
}

class DrawPipeline {
  vertexShader(VertexBuiltins^ vb) vertex {
    var matrix = bindings.Get().uniforms.MapReadUniform().matrix;
    vb.position = matrix * Utils.makeFloat4(vertices.Get());
  }
  fragmentShader(FragmentBuiltins^ fb) fragment {
    fragColor.Set(bindings.Get().uniforms.MapReadUniform().color);
  }
  var vertices : vertex Buffer<Vector[]>*;
  var fragColor : ColorAttachment<PreferredSwapChainFormat>*;
  var bindings : BindGroup<Bindings>*;
}

var bodies = new Body[width * height * depth];
var springs = new Spring[bodies.length * 3 - width * depth - height * depth - width * height];
var spring = 0;
for (var i = 0; i < bodies.length; ++i) {
  var x = i % width;
  var y = i % (width * height) / width;
  var z = i / (width * height);
  var pos = Utils.makeVector((float) (x - width / 2) + 0.5,
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

var device = new Device();
var window = new Window({0, 0}, {960, 960});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var physicsSystem = new ParticleSystem(bodies, springs);

var bodyVerts = new Vector[bodies.length * 3];
var bodyVBO = new vertex Buffer<Vector[]>(device, bodyVerts.length);

var springVerts = new Vector[springs.length * 2];
var springVBO = new vertex Buffer<Vector[]>(device, springVerts.length);

var bodyPipeline = new RenderPipeline<DrawPipeline>(device, null, TriangleList);
var springPipeline = new RenderPipeline<DrawPipeline>(device, null, LineList);
var bodyBindings : Bindings;
bodyBindings.uniforms = new uniform Buffer<DrawUniforms>(device);
var bodyBG = new BindGroup<Bindings>(device, &bodyBindings);
var springBindings : Bindings;
springBindings.uniforms = new uniform Buffer<DrawUniforms>(device);
var springBG = new BindGroup<Bindings>(device, &springBindings);
var handler : EventHandler;
handler.rotation = float<2>(0.0, 0.0);
handler.distance = 0.5 * (float) width;
var drawUniforms : DrawUniforms;
var projection = Transform.projection(1.0, 100.0, -1.0, 1.0, -1.0, 1.0);
var startTime = System.GetCurrentTime();
var frequency = 480.0;
var maxStepsPerFrame = 32;
var stepsDone = 0;
while(System.IsRunning()) {
  var orientation = Quaternion(float<3>(0.0, 1.0, 0.0), handler.rotation.x);
  orientation = orientation.mul(Quaternion(float<3>(1.0, 0.0, 0.0), handler.rotation.y));
  orientation.normalize();
  drawUniforms.matrix = projection;
  drawUniforms.matrix *= Transform.translate(0.0, 0.0, -handler.distance);
  drawUniforms.matrix *= orientation.toMatrix();
  drawUniforms.color = float<4>(1.0, 1.0, 1.0, 1.0);
  springBindings.uniforms.SetData(&drawUniforms);
  drawUniforms.color = float<4>(0.0, 1.0, 0.0, 1.0);
  bodyBindings.uniforms.SetData(&drawUniforms);
  for (var i = 0; i < bodies.length; ++i) {
    var p = bodies[i].position;
    bodyVerts[i*3  ] = p + Vector( 0.1,  0.0);
    bodyVerts[i*3+1] = p + Vector(-0.1,  0.0);
    bodyVerts[i*3+2] = p + Vector( 0.0, -0.2);
  }
  bodyVBO.SetData(bodyVerts);

  for (var i = 0; i < springs.length; ++i) {
    springVerts[i*2] = bodies[springs[i].body1].position;
    springVerts[i*2+1] = bodies[springs[i].body2].position;
  }
  springVBO.SetData(springVerts);

  var totalSteps = (int) ((System.GetCurrentTime() - startTime) * frequency);

  for (var i = 0; stepsDone < totalSteps && i < maxStepsPerFrame; i++) {
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
