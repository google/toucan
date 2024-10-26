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
  var spring : [6]int;
  var springWeight : [6]float;

  computeAcceleration() {
    acceleration = force / mass;
  }

  eulerStep(deltaT : float) {
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
  var force : Vector;

  computeForce(b1 : Body, b2 : Body) : Vector {
    var dp = b1.position - b2.position;
    var dv = b1.velocity - b2.velocity;
    var dplen = Utils.length(dp);
    var f = ks * (dplen - r) + kd * Utils.dot(dv, dp) / dplen;
    return -dp / dplen * f;
  }

  Spring(b1 : int, b2 : int) {
    body1 = b1;
    body2 = b2;
    ks = 1.0;
    kd = 0.4;
    r = 0.3;
  }
}

class ComputeUniforms {
  var gravity : Vector;
  var wind : Vector;
  var deltaT : float;
}

class DrawUniforms {
  var matrix : float<4,4>;
  var color : float<4>;
}

class ComputeBindings {
  var bodyStorage : *storage Buffer<[]Body>;
  var springStorage : *storage Buffer<[]Spring>;
  var bodyVerts : *storage Buffer<[]Vector>;
  var springVerts : *storage Buffer<[]Vector>;
  var uniforms : *uniform Buffer<ComputeUniforms>;
}

class ComputeBase {
  var bindings : *BindGroup<ComputeBindings>;
}

class ComputeForces : ComputeBase {
  compute(1, 1, 1) main(cb : ^ComputeBuiltins) {
    var bodies = bindings.Get().bodyStorage.Map();
    var springs = bindings.Get().springStorage.Map();
    var u = bindings.Get().uniforms.Map();
    var i = cb.globalInvocationId.x;
    var spring = springs[i];
    var body1 = bodies[spring.body1];
    var body2 = bodies[spring.body2];
    springs[i].force = spring.computeForce(body1, body2);
  }
}

class ApplyForces : ComputeBase {
  compute(1, 1, 1) main(cb : ^ComputeBuiltins) {
    var bodies = bindings.Get().bodyStorage.Map();
    var springs = bindings.Get().springStorage.Map();
    var u = bindings.Get().uniforms.Map();
    var i = cb.globalInvocationId.x;
    var body = bodies[i];
    body.force = u.gravity + u.wind;
    for (var i = 0; i < 6; ++i) {
      body.force += springs[body.spring[i]].force * body.springWeight[i];
    }
    body.computeAcceleration();
    body.eulerStep(u.deltaT);
    bodies[i] = body;
  }
}

class UpdateBodyVerts : ComputeBase {
  compute(1, 1, 1) main(cb : ^ComputeBuiltins) {
    var bodies = bindings.Get().bodyStorage.Map();
    var i = cb.globalInvocationId.x;
    var p = bodies[i].position;
    var bv = bindings.Get().bodyVerts.Map();
    bv[i*3]   = p + Vector( 0.1,  0.0);
    bv[i*3+1] = p + Vector(-0.1,  0.0);
    bv[i*3+2] = p + Vector( 0.0, -0.2);
  }
}

class UpdateSpringVerts : ComputeBase {
  compute(1, 1, 1) main(cb : ^ComputeBuiltins) {
    var bodies = bindings.Get().bodyStorage.Map();
    var springs = bindings.Get().springStorage.Map();
    var sv = bindings.Get().springVerts.Map();
    var i = cb.globalInvocationId.x;
    sv[i*2] = bodies[springs[i].body1].position;
    sv[i*2+1] = bodies[springs[i].body2].position;
  }
}

class DrawBindings {
  var uniforms : *uniform Buffer<DrawUniforms>;
}

class DrawPipeline {
  vertex main(vb : ^VertexBuiltins) {
    var matrix = bindings.Get().uniforms.Map().matrix;
    vb.position = matrix * Utils.makeFloat4(vertices.Get());
  }
  fragment main(fb : ^FragmentBuiltins) {
    fragColor.Set(bindings.Get().uniforms.Map().color);
  }
  var vertices : *vertex Buffer<[]Vector>;
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
  var bindings : *BindGroup<DrawBindings>;
}

var bodies = (width * height * depth) new Body;
var springs = (bodies.length * 3 - width * depth - height * depth - width * height) new Spring;
var spring = 0;
for (var i = 0; i < bodies.length; ++i) {
  bodies[i].springWeight[0] = 0.0;
  bodies[i].springWeight[1] = 0.0;
  bodies[i].springWeight[2] = 0.0;
  bodies[i].springWeight[3] = 0.0;
  bodies[i].springWeight[4] = 0.0;
  bodies[i].springWeight[5] = 0.0;
}

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

  var body1 = i;
  if (x < width - 1) {
    var body2 = i + 1;
    springs[spring] = Spring(body1, body2);
    bodies[body1].spring[0] = spring;
    bodies[body2].spring[3] = spring;
    bodies[body1].springWeight[0] = 1.0;
    bodies[body2].springWeight[3] = -1.0;
    ++spring;
  }

  if (y < height - 1) {
    var body2 = i + width;
    springs[spring] = Spring(body1, body2);
    bodies[body1].spring[1] = spring;
    bodies[body2].spring[4] = spring;
    bodies[body1].springWeight[1] = 1.0;
    bodies[body2].springWeight[4] = -1.0;
    ++spring;
  }

  if (z < depth - 1) {
    var body2 = i + width * height;
    springs[spring] = Spring(body1, body2);
    bodies[body1].spring[2] = spring;
    bodies[body2].spring[5] = spring;
    bodies[body1].springWeight[2] = 1.0;
    bodies[body2].springWeight[5] = -1.0;
    ++spring;
  }
}

var device = new Device();
var window = new Window({0, 0}, {960, 960});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);

var numBodyVerts = bodies.length * 3;
var bodyVBO = new vertex storage Buffer<[]Vector>(device, numBodyVerts);
var numSpringVerts = springs.length * 2;
var springVBO = new vertex storage Buffer<[]Vector>(device, numSpringVerts);

var computeUBO = new uniform Buffer<ComputeUniforms>(device);

var computeBindGroup = new BindGroup<ComputeBindings>(device, {
  bodyVerts = bodyVBO,
  springVerts = springVBO,
  uniforms = computeUBO,
  bodyStorage = new storage Buffer<[]Body>(device, bodies),
  springStorage = new storage Buffer<[]Spring>(device, springs)
});

var bodyPipeline = new RenderPipeline<DrawPipeline>(device, null, TriangleList);
var springPipeline = new RenderPipeline<DrawPipeline>(device, null, LineList);
var computeForces = new ComputePipeline<ComputeForces>(device);
var applyForces = new ComputePipeline<ApplyForces>(device);
var updateBodyVerts = new ComputePipeline<UpdateBodyVerts>(device);
var updateSpringVerts = new ComputePipeline<UpdateSpringVerts>(device);
var frequency = 480.0;
var maxStepsPerFrame = 32;
var stepsDone = 0;
var bodyUBO = new uniform Buffer<DrawUniforms>(device);
var bodyBindings : DrawBindings;
bodyBindings.uniforms = bodyUBO;
var bodyBG = new BindGroup<DrawBindings>(device, &bodyBindings);
var springUBO = new uniform Buffer<DrawUniforms>(device);
var springBindings : DrawBindings;
springBindings.uniforms = springUBO;
var springBG = new BindGroup<DrawBindings>(device, &springBindings);
var handler : EventHandler;
handler.rotation = float<2>(0.0, 0.0);
handler.distance = 0.5 * (float) width;
var computeUniforms : ComputeUniforms;
var projection = Transform.projection(1.0, 100.0, -1.0, 1.0, -1.0, 1.0);
computeUniforms.deltaT = 8.0 / frequency;
computeUniforms.gravity = Vector(0.0, -0.25);
var startTime = System.GetCurrentTime();
while(System.IsRunning()) {
  var orientation = Quaternion(float<3>(0.0, 1.0, 0.0), handler.rotation.x);
  orientation = orientation.mul(Quaternion(float<3>(1.0, 0.0, 0.0), handler.rotation.y));
  orientation.normalize();
  var matrix = projection;
  matrix *= Transform.translate(0.0, 0.0, -handler.distance);
  matrix *= orientation.toMatrix();

  springUBO.SetData({matrix = matrix, color = {1.0, 1.0, 1.0, 1.0}});
  bodyUBO.SetData({matrix = matrix, color = {0.0, 1.0, 0.0, 1.0}});

  computeUniforms.wind = Vector(Math.rand() * 0.01, 0.0);
  computeUBO.SetData(&computeUniforms);

  var encoder = new CommandEncoder(device);
  var computePass = new ComputePass<ComputeBase>(encoder, {bindings = computeBindGroup});

  var totalSteps = (int) ((System.GetCurrentTime() - startTime) * frequency);
  for (var i = 0; stepsDone < totalSteps && i < maxStepsPerFrame; i++) {
    var computeForcesPass = new ComputePass<ComputeForces>(computePass);
    computeForcesPass.SetPipeline(computeForces);
    computeForcesPass.Dispatch(springs.length, 1, 1);

    var applyForcesPass = new ComputePass<ApplyForces>(computePass);
    applyForcesPass.SetPipeline(applyForces);
    applyForcesPass.Dispatch(bodies.length, 1, 1);
    stepsDone++;
  }

  var updateBodyPass = new ComputePass<UpdateBodyVerts>(computePass);
  updateBodyPass.SetPipeline(updateBodyVerts);
  updateBodyPass.Dispatch(bodies.length, 1, 1);

  var updateSpringPass = new ComputePass<UpdateSpringVerts>(computePass);
  updateSpringPass.SetPipeline(updateSpringVerts);
  updateSpringPass.Dispatch(springs.length, 1, 1);

  computePass.End();
  var p : DrawPipeline;
  p.fragColor = swapChain.GetCurrentTexture().CreateColorAttachment(Clear, Store);
  var renderPass = new RenderPass<DrawPipeline>(encoder, &p);

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
