include "include/test.t"

enum Enum1 { FOO, BAR, BAZ };
class A {
  void init() {
    f = FOO;
  }
  Enum1 f;
};
A* a = new A();
a.init();
Test.Expect(a.f == FOO);
a.f = BAR;
Test.Expect(a.f == BAR);
float r = 0.0;
if (a.f == BAR ) {
  r = 1.0;
}
Test.Expect(r == 1.0);
return r;
