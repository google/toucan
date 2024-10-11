include "include/test.t"

class Template<T> {
  var foo : T<4>;
  Template(t : T<4>) { foo = t; }
  get() : T<4> { return foo; }
}

var temp  = new Template<float>(float<4>(1.0, 2.0, 3.0, 4.0));
var f : float<4> = temp.get();
Test.Expect(f.x == 1.0);
Test.Expect(f.y == 2.0);
Test.Expect(f.z == 3.0);
Test.Expect(f.w == 4.0);
