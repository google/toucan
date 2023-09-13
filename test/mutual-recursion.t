class Foo {
  float y;
  void foo(int x) {
    if (x > 0) this.bar(x - 1);
    return;
  }
  void bar(int x) {
    y += 1.0;
    this.foo(x);
    return;
  }
};

Foo* foo = new Foo();
foo.y = 0.0;
foo.foo(10);
return foo.y;
