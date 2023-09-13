class Foo {
  void cs1() compute(1) {}
  void cs21() compute(2, 1) {}
  void cs321() compute(3, 2, 1) {}
  void fail_cs0() compute() {}
  void fail_cs1234() compute(1, 2, 3, 4) {}
  void fail_cs1() compute(false) {}
  void fail_fs1() fragment(1) {}
  void fail_vs1() vertex(1) {}
};
