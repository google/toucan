class Base {
  vertex main(vb : &VertexBuiltins) {
    vb.position = {1.0, 0.0, 0.0, 1.0};
  }
}

class C<PF> : Base {
  fragment main(fb : &FragmentBuiltins) {
    fragColor.Set({0.0, 1.0, 0.0, 1.0});
  }
  var fragColor : *ColorOutput<PF>;
}

var p = new RenderPipeline<C<RGBA8unorm>>;
