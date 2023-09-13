class Foo {
  void inc() {
    x = x + 1.0;
    return;
  }
  float x;
};

Foo* foo = new Foo();
foo.x = 0.0;
foo.inc();
foo.inc();
foo.inc();
return foo.x;
