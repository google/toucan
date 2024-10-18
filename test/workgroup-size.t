class Foo {
  compute(1) cs1() {}
  compute(2, 1) cs21() {}
  compute(3, 2, 1) cs321() {}
  compute() fail_cs0() {}
  compute(1, 2, 3, 4) fail_cs1234() {}
  compute(false) fail_cs1() {}
  fragment(1) fail_fs1() {}
  vertex(1) fail_vs1() {}
};
