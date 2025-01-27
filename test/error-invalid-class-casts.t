// Test some widening casts
class Base {
};
class Derived : Base {
};
class Base2 {
};
class Derived2 : Base2 {
};

var d = new Derived;
var b2 = new Base2;
var d2 = new Derived2;
var i : int = null;    // error; int cannot be widened to null
var b : *Base = null;  // valid; null can be widened to any pointer
b = d;                 // valid; Derived* can be widened to Base*
b = b2;                // error, b2 is not derived from b
b2 = d2;               // valid; Derived2 can be widened to Base2*
d2 = d;                // error; Derived2 and Derived are unrelated
