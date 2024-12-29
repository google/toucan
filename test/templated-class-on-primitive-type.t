include "include/test.t"

class Template<T> {
  var foo : T;
  set(t : T) { foo = t; }
  get() : T { return foo; }
}

var t = new Template<float>;
t.set(3.0);
Test.Expect(t.get() == 3.0);
