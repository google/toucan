Device* device = new Device();

class Pipeline {
  static float a() {
    return 5.0;
  }
  static float b() {
    return Pipeline.a();
  }
  void computeShader(ComputeBuiltins^ cb) compute(1, 1, 1) {
    auto temp = Pipeline.b();
  }
}

// This passes by generating valid SPIR-V during codegen.
auto tessPipeline = new ComputePipeline<Pipeline>(device);
