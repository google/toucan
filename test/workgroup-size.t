class Foo {
  cs1() compute(1) {}
  cs21() compute(2, 1) {}
  cs321() compute(3, 2, 1) {}
  fail_cs0() compute() {}
  fail_cs1234() compute(1, 2, 3, 4) {}
  fail_cs1() compute(false) {}
  fail_fs1() fragment(1) {}
  fail_vs1() vertex(1) {}
};
