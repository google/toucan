include "include/test.t"

class Foo {
  var x : int;
  var y : float;
};

var foo = [100] new Foo;
foo[23].y = 5.0;
Test.Expect(foo[23].y == 5.0);
