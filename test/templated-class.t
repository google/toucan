include "include/test.t"

class Bar {
  get() : float {
    return 3.0;
  }
}

class Template<T> {
  var foo : T*;
  set(T* t) { foo = t; }
  get() : float { return foo.get(); }
}

var templateBar = new Template<Bar>();
templateBar.set(new Bar());
Test.Expect(templateBar.get() == 3.0);
