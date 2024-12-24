include "include/test.t"

class Foo {
  var r : float;
};
class Bar {
  Bar(f : *Foo) {
    foo = f;
  }
 ~Bar() {
    foo.r = 1234.0;
  }
  var foo : *Foo;
};
var r = -1.0;
{
  var f = new Foo();
  f.r = -1.0;
  {
    var b = new Bar(f);
  }
  r = f.r;
}
Test.Expect(r == 1234.0);
