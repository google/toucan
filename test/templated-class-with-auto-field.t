include "include/test.t"

class C<T> {
  var s1 = 3 as T;
  var s2 : T = 3 as T;
  var v1 = T<2>{};
  var v2 : T<2> = T<2>{};
}

var ci = C<int>{};
Test.Expect(ci.s1 == 3);
Test.Expect(ci.s2 == 3);
Test.Expect(Math.all(ci.v1 == int<2>{}));
Test.Expect(Math.all(ci.v2 == int<2>{}));

var cf = C<float>{};
Test.Expect(cf.s1 == 3.0);
Test.Expect(cf.s2 == 3.0);
Test.Expect(Math.all(cf.v1 == float<2>{}));
Test.Expect(Math.all(cf.v2 == float<2>{}));
