using Vertex = float<4>;

class ComputeBindings {
  var vertStorage : *storage Buffer<[]Vertex>;
}

class BumpCompute {
  compute(1, 1, 1) main(cb : ^ComputeBuiltins) {
    var verts = bindings.Get().vertStorage.Map();
    var pos = cb.globalInvocationId.x;
    if (pos % 2 == 1) {
      verts[pos] += float<4>( 1.0, 0.0, 0.0, 0.0);
    }
  }
  var bindings : *BindGroup<ComputeBindings>;
}

var device = new Device();

// This test passes by producing valid SPIR-V.
var pipeline = new ComputePipeline<BumpCompute>(device);
