include "include/test.t"

class Bar;

var b = new Bar;
b.x = -3.0;
Test.Expect(b.x == -3.0);
return;

class Bar {
  var x : float;
};
