using Vertex = float<4>;

class ComputeBindings {
  storage Buffer<Vertex[]>* vertStorage;
}

class BumpCompute {
  void computeShader(ComputeBuiltins cb) compute(1, 1, 1) {
    auto verts = bindings.Get().vertStorage.MapReadWriteStorage();
    uint pos = cb.globalInvocationId.x;
    if (pos % 2 == 1) {
      verts[pos] += float<4>( 1.0, 0.0, 0.0, 0.0);
    }
  }
  BindGroup<ComputeBindings>* bindings;
}

Device* device = new Device();

// This test passes by producing valid SPIR-V.
auto pipeline = new ComputePipeline<BumpCompute>(device);
