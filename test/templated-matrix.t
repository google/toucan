include "include/test.t"

class Template<T> {
  T<2, 2> foo;
  Template(T<2, 2> t) { foo = t; }
  T<2, 2> get() { return foo; }
}

Template<float>* temp  = new Template<float>(float<2, 2>(float<2>(1.0, 2.0), float<2>(3.0, 4.0)));
float<2, 2> f = temp.get();
Test.Expect(f[0][0] == 1.0);
Test.Expect(f[0][1] == 2.0);
Test.Expect(f[1][0] == 3.0);
Test.Expect(f[1][1] == 4.0);
