include "include/test.t"

class Foo {
  var zzz : [3]int;
  var a : float;
};
var r : float;
{
  var f = new Foo();
  f.a = -1234.0;
  r = f.a;
}
Test.Expect(r == -1234.0);
