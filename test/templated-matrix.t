class Template<T> {
  T<2, 2> foo;
  Template(T<2, 2> t) { foo = t; }
  T<2, 2> get() { return foo; }
}

Template<float>* temp  = new Template<float>(float<2, 2>(float<2>(1.0, 2.0), float<2>(3.0, 4.0)));
float<2, 2> f = temp.get();
return f[1].x;
