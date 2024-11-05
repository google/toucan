include "include/test.t"

var i = 3;
var pi : &int = &i;
pi = 5;
Test.Expect(i == 5);

class C {
  var f : float;
}

var c = C{ 21.0 };
var pf = &c.f;
pf = 42.0;
Test.Expect(c.f == 42.0);

var ii = new int();
ii: = 21;
Test.Expect(ii: == 21);
var pii : &int = ii;
pii = 42;
ii = null;
Test.Expect(pii == 42);
