include "include/test.t"

class Bar {
  Bar() { x = 2.0; }
  bump() : float {
    x += 1.0;
    return x;
  }
  var x : float;
}

class Foo {
  bar() : *Bar {
    return new Bar();
  }
};

class Bump {
}

var foo = new Foo();
Test.Expect(foo.bar().bump() == 3.0);
