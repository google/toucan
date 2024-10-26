using Vector = float<2>;
class Utils;

var width =  10;
var height = 10;
var depth =   1;

class Body {
  computeAcceleration() {
    acceleration = force / mass;
  }

  applyForce(deltaForce : Vector) {
    force += deltaForce;
  }
  var position : Vector;
  var velocity : Vector;
  var force : Vector;
  var acceleration : Vector;
  var spring : [6]int;
  var springWeight : [6]float;
  var mass : float;
  var nailed : float;
}

class Spring {
  Spring(b1 : int, b2 : int) {
    body1 = b1;
    body2 = b2;
    ks = 1.0;
    kd = 0.4;
    r = 0.0;
  }
  computeForce(b1 : Body, b2 : Body) : Vector {
    var dp = b1.position - b2.position;
    var dv = b1.velocity - b2.velocity;
    var dplen = Utils.length(dp);
    var f = ks * (dplen - r) + kd * Utils.dot(dv, dp) / dplen;
    return -dp / dplen * f;
  }

  var body1 : int;
  var body2 : int;
  var ks : float;
  var kd : float;
  var r : float;
  var force : Vector;
}

class Utils {
  static dot(v1 : float<2>, v2 : float<2>) : float {
    var r = v1 * v2;
    return r.x + r.y;
  }
  static length(v : float<2>) : float {
    return Math.sqrt(Utils.dot(v, v));
  }
  static dot(v1 : float<3>, v2 : float<3>) : float {
    var r = v1 * v2;
    return r.x + r.y + r.z;
  }
  static length(v : float<3>) : float {
    return Math.sqrt(Utils.dot(v, v));
  }
  static makeFloat4(v : float<2>) : float<4> {
    return float<4>(v.x, v.y, 0.0, 1.0);
  }
  static makeFloat4(v : float<3>) : float<4> {
    return float<4>(v.x, v.y, v.z, 1.0);
  }
  static makeVector(x : float, y : float, z : float, placeholder : float<2>) : float<2> {
    return float<2>(x, y);
  }
  static makeVector(x : float, y : float, z : float, placeholder : float<3>) : float<3> {
    return float<3>(x, y, z);
  }
}

class Uniforms {
  var gravity : Vector;
  var wind : Vector;
  var particleSize : Vector;
  var deltaT : float;
}

class ComputeBindings {
  var bodyStorage : *storage Buffer<[]Body>;
  var springStorage : *storage Buffer<[]Spring>;
  var bodyVerts : *storage Buffer<[]Vector>;
  var springVerts : *storage Buffer<[]Vector>;
  var uniforms : *uniform Buffer<Uniforms>;
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
    var force = u.gravity + u.wind;
    for (var i = 0; i < 6; ++i) {
      force += springs[body.spring[i]].force * body.springWeight[i];
    }
    bodies[i].force = force;
  }
}

class FinalizeBodies : ComputeBase {
  compute(1, 1, 1) main(cb : ^ComputeBuiltins) {
    var bodies = bindings.Get().bodyStorage.Map();
    var u = bindings.Get().uniforms.Map();
    var deltaT = u.deltaT;
    var i = cb.globalInvocationId.x;
    var body = bodies[i];
    body.computeAcceleration();
    body.position += body.velocity * deltaT * (1.0 - body.nailed);
    body.velocity += body.acceleration * deltaT * (1.0 - body.nailed);
    var p = bodies[i].position;
    var bv = bindings.Get().bodyVerts.Map();
    bodies[i] = body;
    bv[i*3]   = p + Vector( u.particleSize.x * 0.5,  0.0);
    bv[i*3+1] = p + Vector(-u.particleSize.x * 0.5,  0.0);
    bv[i*3+2] = p + Vector( 0.0,           -u.particleSize.y);
  }
}

class FinalizeSprings : ComputeBase {
  compute(1, 1, 1) main(cb : ^ComputeBuiltins) {
    var bodies = bindings.Get().bodyStorage.Map();
    var springs = bindings.Get().springStorage.Map();
    var sv = bindings.Get().springVerts.Map();
    var i = cb.globalInvocationId.x;
    sv[i*2] = bodies[springs[i].body1].position;
    sv[i*2+1] = bodies[springs[i].body2].position;
  }
}

var device = new Device();
var window = new Window({0, 0}, {960, 960});
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var bodies = (width * height * depth) new Body;
var springs = (bodies.length * 3 - width * depth - height * depth - width * height) new Spring;
var count = Utils.makeVector((float) width, (float) height, (float) depth, Vector(0.0));
var pSpacing = Vector(2.0) / count;
var spring = 0;

for (var i = 0u; i < bodies.length; ++i) {
  bodies[i].springWeight[0] = 0.0;
  bodies[i].springWeight[1] = 0.0;
  bodies[i].springWeight[2] = 0.0;
  bodies[i].springWeight[3] = 0.0;
  bodies[i].springWeight[4] = 0.0;
  bodies[i].springWeight[5] = 0.0;
}

for (var i = 0u; i < bodies.length; ++i) {
  var x = i % width;
  var y = i % (width * height) / width;
  var z = i / (width * height);
  var pos = Utils.makeVector((float) x, (float) y, (float) z, Vector(0.0));
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

  var body1 = i;
  var body2 = i + 1;
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


var numBodyVerts = bodies.length * 3;
var bodyVBO = new vertex storage Buffer<[]Vector>(device, numBodyVerts);
var numSpringVerts = springs.length * 2;
var springVBO = new vertex storage Buffer<[]Vector>(device, numSpringVerts);

var computeUBO = new uniform Buffer<Uniforms>(device);

var computeBindGroup = new BindGroup<ComputeBindings>(device, {
  bodyVerts = bodyVBO,
  springVerts = springVBO,
  uniforms = computeUBO,
  bodyStorage = new storage Buffer<[]Body>(device, bodies),
  springStorage = new storage Buffer<[]Spring>(device, springs)
});
class Shaders {
  var vert : *vertex Buffer<[]Vector>;
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
}

class BodyShaders : Shaders {
  vertex main(vb : ^VertexBuiltins) { vb.position = Utils.makeFloat4(vert.Get()); }
  fragment main(fb : ^FragmentBuiltins) { fragColor.Set(float<4>(0.0, 1.0, 0.0, 1.0)); }
}

class SpringShaders : Shaders {
  vertex main(vb : ^VertexBuiltins) { vb.position = Utils.makeFloat4(vert.Get()); }
  fragment main(fb : ^FragmentBuiltins) { fragColor.Set(float<4>(1.0, 1.0, 1.0, 1.0)); }
}

var bodyPipeline = new RenderPipeline<BodyShaders>(device, null, TriangleList);
var springPipeline = new RenderPipeline<SpringShaders>(device, null, LineList);
var computeForces = new ComputePipeline<ComputeForces>(device);
var applyForces = new ComputePipeline<ApplyForces>(device);
var finalizeBodies = new ComputePipeline<FinalizeBodies>(device);
var finalizeSprings = new ComputePipeline<FinalizeSprings>(device);
var frequency = 240.0; // physics sim at 240Hz
System.GetNextEvent();
var u : Uniforms;
u.deltaT = 1.0 / frequency;
u.gravity = Vector(0.0, 0.0);
u.particleSize = Vector(0.5) / (float) width;
while(System.IsRunning()) {
  u.wind = Vector(Math.rand() * 0.00, 0.0);
  computeUBO.SetData(&u);

  var encoder = new CommandEncoder(device);

  var computePass = new ComputePass<ComputeBase>(encoder, {bindings = computeBindGroup} );

  var computeForcesPass = new ComputePass<ComputeForces>(computePass);
  computeForcesPass.SetPipeline(computeForces);
  computeForcesPass.Dispatch(springs.length, 1, 1);

  var applyForcesPass = new ComputePass<ApplyForces>(computePass);
  applyForcesPass.SetPipeline(applyForces);
  applyForcesPass.Dispatch(bodies.length, 1, 1);

  var finalizeBodiesPass = new ComputePass<FinalizeBodies>(computePass);
  finalizeBodiesPass.SetPipeline(finalizeBodies);
  finalizeBodiesPass.Dispatch(bodies.length, 1, 1);

  var finalizeSpringsPass = new ComputePass<FinalizeSprings>(computePass);
  finalizeSpringsPass.SetPipeline(finalizeSprings);
  finalizeSpringsPass.Dispatch(springs.length, 1, 1);

  computePass.End();
  var colorAttachment = swapChain.GetCurrentTexture().CreateColorAttachment(Clear, Store);
  var renderPass = new RenderPass<Shaders>(encoder, { fragColor = colorAttachment });

  var bodyPass = new RenderPass<BodyShaders>(renderPass);
  bodyPass.SetPipeline(bodyPipeline);
  bodyPass.Set({vert = bodyVBO});
  bodyPass.Draw(numBodyVerts, 1, 0, 0);

  var springPass = new RenderPass<SpringShaders>(renderPass);
  springPass.SetPipeline(springPipeline);
  springPass.Set({vert = springVBO});
  springPass.Draw(numSpringVerts, 1, 0, 0);

  renderPass.End();
  device.GetQueue().Submit(encoder.Finish());
  swapChain.Present();
}
