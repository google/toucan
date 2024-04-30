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
return foobar.mine.GetFloat() + foofloat.mine;
