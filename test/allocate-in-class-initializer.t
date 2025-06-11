include "test.t"

class C {
  var a : *int;
}

var c = C{ new int };

c.a: = 42;
Test.Expect(c.a: == 42);
