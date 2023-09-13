class Template<T> {
  T<4> foo;
  Template(T<4> t) { foo = t; }
  T<4> get() { return foo; }
}

Template<float>* temp  = new Template<float>(float<4>(1.0, 2.0, 3.0, 4.0));
float<4> f = temp.get();
return f.y;
