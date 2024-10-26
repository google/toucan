class Pipeline {
  vertex main(vb : ^VertexBuiltins) {
    vb.position = vertices.Get();
    var i = indices.Get();
  }
  fragment main(fb : ^FragmentBuiltins) {
    fragColor.Set( {0.0, 1.0, 0.0, 1.0} );
  }
  var vertices : *vertex Buffer<[]float<4>>;
  var indices : *index Buffer<[]uint>;
  var fragColor : *ColorAttachment<PreferredSwapChainFormat>;
}
