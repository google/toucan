include "include/test.t"

class C {
  var i = 1;
}

class D {
  var c : C;
}

var c : C;
Test.Expect(c.i == 1);

var d : D = {};
Test.Expect(d.c.i == 1);
