include "include/test.t"

class Foo {
  var i : int;
  var f = 42.0;
};

var foo : Foo;
Test.Expect(foo.f == 42.0);
