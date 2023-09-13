class Foo {
  float f;
  float<4> vec;
};

Foo* foo = new Foo();
foo.vec = float<4>(1.0, 2.0, 3.0, 4.0);
return foo.vec.z;
