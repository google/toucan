include "include/test.t"

enum Enum1 { FOO, BAR, BAZ };
class A {
  void init() {
    f = FOO;
  }
  var f : Enum1;
};
var a = new A();
a.init();
Test.Expect(a.f == FOO);
a.f = BAR;
Test.Expect(a.f == BAR);
var r = 0.0;
if (a.f == BAR ) {
  r = 1.0;
}
Test.Expect(r == 1.0);
