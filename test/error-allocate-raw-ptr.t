class C {
  var f : float;
  var i : &int;
}

var p0 = new &int;
var p1 = new *&int;
var p2 = new ^&int;
var p3 = new [3]&int;
var p4 = new C{};
var p5 = new readonly C{};
