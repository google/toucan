include "include/test.t"

class C {
  float f;
  int[] array;
};
C* c = new [5]C();
c.array[2] = 42;

Test.Expect(c.array[2] == 42);
