class BadPipelineField {
  vertex main(vb : &VertexBuiltins) {}
  fragment main(fb : &FragmentBuiltins) {}
  var bad : int;
}

class NoVertexShader {
  fragment main(fb : &FragmentBuiltins) {}
}

class NoFragmentShader {
  vertex main(vb : &VertexBuiltins) {}
}

class InheritedVertexShaderGood : NoFragmentShader {
  fragment main(fb : &FragmentBuiltins) {}
}

class InheritedFragmentShaderGood : NoVertexShader {
  vertex main(vb : &VertexBuiltins) {}
}

class NoShaders {
}

class NoShaders2 {
}

var device = new Device();
var encoder = new CommandEncoder(device);

new RenderPipeline<BadPipelineField>(device);
new RenderPass<BadPipelineField>(encoder, {});
new RenderPipeline<NoVertexShader>(device);
new RenderPipeline<NoFragmentShader>(device);
new RenderPipeline<NoShaders>(device);
var a : *RenderPipeline<NoShaders2>;
new RenderPipeline<InheritedFragmentShaderGood>(device); // this should pass
new RenderPipeline<InheritedVertexShaderGood>(device);   // this should pass
new RenderPass<NoShaders>(encoder, {});                  // this should pass (no shaders neeeded)
