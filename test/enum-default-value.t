include "include/test.t"

enum Enum { Foo, Bar, Baz };
class A {
  var f = Enum.Bar;
};
var a : A;
Test.Expect(a.f == Enum.Bar);
a.f = Enum.Foo;
Test.Expect(a.f == Enum.Foo);
