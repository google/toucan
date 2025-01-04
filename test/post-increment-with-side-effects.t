include "include/test.t"

class Bar {
  var a : int;
}

class Foo {
  Foo() { bar = new Bar; }
  GetBar() : ^Bar { count += 1; return bar; }
  var bar : *Bar;
  var count : int;
}

var foo = new Foo();
foo.GetBar().a++;

Test.Expect(foo.bar.a == 1);
Test.Expect(foo.count == 1);
