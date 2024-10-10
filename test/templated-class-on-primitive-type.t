include "include/test.t"

class Template<T> {
  var foo : T;
  void set(T t) { foo = t; }
  T get() { return foo; }
}

var t = new Template<float>();
t.set(3.0);
Test.Expect(t.get() == 3.0);
