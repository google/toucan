include "include/test.t"

class Foo {
  var x : int;
  var y : float;
};

var foo = new Foo[100];
foo[23].y = 5.0;
Test.Expect(foo[23].y == 5.0);
