var device = new Device();

class Pipeline {
  static a() : float {
    return 5.0;
  }
  static b() : float {
    return Pipeline.a();
  }
  computeShader(ComputeBuiltins^ cb) compute(1, 1, 1) {
    var temp = Pipeline.b();
  }
}

// This passes by generating valid SPIR-V during codegen.
var tessPipeline = new ComputePipeline<Pipeline>(device);
