include "include/test.t"

class C {
  var x : int = 42;
  var y : int = 21;
}

var c1 : C;
Test.Expect(c1.x == 42);
Test.Expect(c1.y == 21);

var c2 = new C;
Test.Expect(c2.x == 42);
Test.Expect(c2.y == 21);
