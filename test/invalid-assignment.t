float = 3;
class Foo {
  int bar() {
    return 0;
  }
  baz() {
    this = this;
  }
}
Foo* foo = new Foo();
foo.bar() = 5;
