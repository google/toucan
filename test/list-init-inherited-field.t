include "include/test.t"

class C {
  var i : int;
  var a = 42.0;
};

class D : C {
}

var d : D = { i = 3, a = 5.0 };
Test.Expect(d.i == 3);
Test.Expect(d.a == 5.0);
