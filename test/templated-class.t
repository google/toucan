class Bar {
  float get() {
    return 3.0;
  }
}

class Template<T> {
  T* foo;
  void set(T* t) { foo = t; }
  float get() { return foo.get(); }
}

Template<Bar>* templateBar = new Template<Bar>();
templateBar.set(new Bar());
return templateBar.get();
