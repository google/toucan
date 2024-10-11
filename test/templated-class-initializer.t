include "include/test.t"

class S {
  S(v : float) {
    value = v;
  }
  var value : float;
}

class Template<T> {
  var value : T = T(5.0);
}
var foo : Template<S>;
Test.Expect(foo.value.value == 5.0);
