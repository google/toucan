class Foo {
  bar(i : int) uniform;
};

var f : Foo;
f.bar(i = 42);
f.bar(42);
