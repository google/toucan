include "include/test.t"

var x = 3.0;
var y : float;
{
  var z = x * 2.0;
  y = z;
}
Test.Expect(y == 6.0);
