include "include/test.t"

class C {
  C(float v = 3.0) {
    value = v;
  }
  float value;
}

C* c = new C();
Test.Expect(c.value == 3.0);
