include "include/test.t"

class S {
  S(float v) {
    value = v;
  }
  float value;
}

class Template<T> {
  T value = T(5.0);
}
Template<S> foo;
Test.Expect(foo.value.value == 5.0);
return foo.value.value;
