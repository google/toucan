include "include/test.t"

class Foo<T> {
  T mine;
  Foo(T t) {
    auto s = t;
    mine = s;
  }
}

class Bar {
  static float GetFloat() {
    return 5.0;
  }
}

Foo<float> foofloat = Foo<float>(3.0);
Foo<Bar> foobar;
Test.Expect(foobar.mine.GetFloat() == 5.0);
Test.Expect(foofloat.mine == 3.0);
