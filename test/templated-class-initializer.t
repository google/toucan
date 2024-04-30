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
return foo.value.value;
