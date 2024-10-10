include "include/test.t"

var v21 = float<2>{1.0};
var v22 = float<2>{2.0, 3.0};

Test.Expect(v21.x == 1.0);
Test.Expect(v21.y == 1.0);
Test.Expect(v22.x == 2.0);
Test.Expect(v22.y == 3.0);
