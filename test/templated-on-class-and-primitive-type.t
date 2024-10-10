include "include/test.t"

class Foo<T> {
  var mine : T;
  Foo(T t) {
    var s = t;
    mine = s;
  }
}

class Bar {
  static GetFloat() : float {
    return 5.0;
  }
}

var foofloat = Foo<float>(3.0);
var foobar : Foo<Bar>;
Test.Expect(foobar.mine.GetFloat() == 5.0);
Test.Expect(foofloat.mine == 3.0);
