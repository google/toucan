include "include/test.t"

enum Enum { Foo, Bar, Baz };
class A {
  init() {
    f = Enum.Foo;
  }
  var f : Enum;
};
var a = new A;
a.init();
Test.Expect(a.f == Enum.Foo);
a.f = Enum.Bar;
Test.Expect(a.f == Enum.Bar);
var r = 0.0;
if (a.f == Enum.Bar ) {
  r = 1.0;
}
Test.Expect(r == 1.0);
