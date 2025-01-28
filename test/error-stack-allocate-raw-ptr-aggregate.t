class C {
  var pi : &int;
}

class D {
  var c : readonly C;
}

var i = 0;
var ok = &i;
var e0 = [3]&int{};
var e1 : [3]&int;
var e2 = C{};
var e3 : C;
var e4 : readonly C;
var e5 : D;
var e6 : [3]C;
var e6 : [3]D;
