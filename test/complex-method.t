include "include/test.t"

class Foo {
  static float dot(float<4> arg1, float<4> arg2) {
    float<4> tmp = arg1 * arg2;
    return tmp.x + tmp.y + tmp.z + tmp.w;
  }
};

Foo* foo = new Foo();
float result = foo.dot(float<4>(1.0, 0.5, 0.0, 1.0), float<4>(0.5, 1.0, 0.0, 9.0));
Test.Expect(result == 10.0);
