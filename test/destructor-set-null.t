include "include/test.t"

class Bar;

class Foo {
  Foo(count : ^int) : { count = count } {
    count:++;
  }
 ~Foo() {
    count:--;
  }

  var count : ^int;
};

var count = new int;
Test.Expect(count: == 0);
var foo = new Foo(count);
Test.Expect(count: == 1);
foo = null;
Test.Expect(count: == 0);

{
  var foo = Foo(count);
  Test.Expect(count: == 1);
}
Test.Expect(count: == 0);
  
