class Shader {
    vertexMain(float<4> pos) vertex : float<4> {
        return matrix * pos;
    }
    fragmentMain (float<4> pos) fragment : float<4> {
        return float<4>(0.0, 1.0, 0.0, 1.0);
    }
    class {
      var matrix : float<4><4>;
    } objectdata;
};
RenderPipelineDescriptor descriptor;
descriptor.vertexModule = device.CreateShaderModule("Shader", "vertex");
descriptor.fragmentModule = device.CreateShaderModule("Shader", "fragment");
var pipeline = new RenderPipeline<RenderPipelineDescriptor>(device, null, TriangleList);
