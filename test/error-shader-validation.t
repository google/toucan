class NoNewInShaders {
  compute(1) main(cb : &ComputeBuiltins) {
    var i = new int;
    var a : [3]float;
    var slice = &a[1 .. 3];
  }
}

var device = new Device();

new ComputePipeline<NoNewInShaders>(device);
