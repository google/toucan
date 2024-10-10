include "include/test.t"

var a = 3.0;
class Foo {
  void^ Weak() { return new Foo(); }
  void* Strong() { return new Foo(); }
  void^ WeakNull() { return null; }
  void* StrongNull() { return null; }
}
var f = new Foo();
f.Weak();
f.Strong();
f.WeakNull();
f.StrongNull();
Test.Expect(a == 3.0);
