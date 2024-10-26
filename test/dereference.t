include "include/test.t"

var f = new float();
*f = 3.0;
var v = new float<3>();
*v = float<3>(2.0, 4.0, 6.0);
var temp = *v;
var a = [3] new float;
*a = [3]float(3.0, 2.0, 1.0);
Test.Expect(*f == 3.0);
Test.Expect(temp.x == 2.0);
Test.Expect(temp.y == 4.0);
Test.Expect(temp.z == 6.0);
Test.Expect(a[0] == 3.0);
Test.Expect(a[1] == 2.0);
Test.Expect(a[2] == 1.0);
