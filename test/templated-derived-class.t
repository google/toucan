include "include/test.t"

class Base<T> {
  Foo() : T {
    return 42 as T;
  }
}

class Derived<T> : Base<T> {
}

var ci = new Derived<int>;
Test.Expect(ci.Foo() == 42);

var df = new Derived<float>;
Test.Expect(df.Foo() == 42.0);
