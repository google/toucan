class NoNewInShaders {
  compute(1) main(cb : &ComputeBuiltins) { var i = new int; }
}

var device = new Device();

new ComputePipeline<NoNewInShaders>(device);
