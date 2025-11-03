include "include/test.t"

class Base<T> {
  Foo() : T {
    return (T) 42;
  }
}

class Derived<T> : Base<T> {
}

var ci = new Derived<int>;
Test.Expect(ci.Foo() == 42);

var df = new Derived<float>;
Test.Expect(df.Foo() == 42.0);
