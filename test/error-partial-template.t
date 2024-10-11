class Foo<T, U> {
  var t : T;
  var u : U;
}

var full : Foo<int, int>*;
var partial : Foo<int>*;
var noargs : Foo*;

partial = full;
noargs = full;
