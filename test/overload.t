include "include/test.t"

class Foo {
  float bar(int a) {
    float f = 0.0;
    for (; a > 0; --a) {
      f += 1.0;
    }
    return f;
  }
  float bar(float a) {
    return 0.0-a;
  }
};

Foo* f = new Foo();
Test.Expect(f.bar(3) == 3.0);
Test.Expect(f.bar(-4.0) == 4.0);
