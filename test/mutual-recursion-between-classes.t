class Bar;

class Foo {
  foo() {
    bar.bar();
  }
  var bar : *Bar;
};

class Bar {
  bar() {
    foo.foo();
  }
  var foo : *Foo;
};
