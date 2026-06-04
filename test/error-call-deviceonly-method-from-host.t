#include "include/test.t"

class C {
  deviceonly Foo() {
  }
  Bar() {
    Foo();
  }
}

var c = new C;
