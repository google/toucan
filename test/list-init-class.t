include "include/test.t"

class C {
  var i : int;
  var a = 42.0;
};

var c = C{ 3, 5.0 };
Test.Expect(c.i == 3);
Test.Expect(c.a == 5.0);
