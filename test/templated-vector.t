include "include/test.t"

class Template<T> {
  T<4> foo;
  Template(T<4> t) { foo = t; }
  T<4> get() { return foo; }
}

Template<float>* temp  = new Template<float>(float<4>(1.0, 2.0, 3.0, 4.0));
float<4> f = temp.get();
Test.Expect(f.x == 1.0);
Test.Expect(f.y == 2.0);
Test.Expect(f.z == 3.0);
Test.Expect(f.w == 4.0);
return f.y;
