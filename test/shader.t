class Shader {
    float<4> vertexMain(float<4> pos) vertex {
        return matrix * pos;
    }
    float<4> fragmentMain (float<4> pos) fragment {
        return float<4>(0.0, 1.0, 0.0, 1.0);
    }
    class {
      float<4><4> matrix;
    } objectdata;
};
RenderPipelineDescriptor descriptor;
descriptor.vertexModule = device.CreateShaderModule("Shader", "vertex");
descriptor.fragmentModule = device.CreateShaderModule("Shader", "fragment");
var pipeline = new RenderPipeline<RenderPipelineDescriptor>(device, null, TriangleList);
