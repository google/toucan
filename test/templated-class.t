include "include/test.t"

class Bar {
  float get() {
    return 3.0;
  }
}

class Template<T> {
  var foo : T*;
  void set(T* t) { foo = t; }
  float get() { return foo.get(); }
}

var templateBar = new Template<Bar>();
templateBar.set(new Bar());
Test.Expect(templateBar.get() == 3.0);
