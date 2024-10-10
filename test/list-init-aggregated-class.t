include "include/test.t"

class C {
  var i : int;
  var a = 42.0;

};

class D {
  var c : C;
  var f : float;
}

var d : D = { { 3, 5.0 }, 2.0 };
Test.Expect(d.c.i == 3);
Test.Expect(d.c.a == 5.0);
Test.Expect(d.f == 2.0);
