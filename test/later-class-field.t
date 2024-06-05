include "include/test.t"

class Bar;

Bar* b = new Bar();
b.x = -3.0;
Test.Expect(b.x == -3.0);
return b.x;

class Bar {
  float x;
};
