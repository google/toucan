// Test some widening casts
class Base {
};
class Derived : Base {
};
class Base2 {
};
class Derived2 : Base2 {
};

Derived* d = new Derived();
Base2* b2 = new Base2();
Derived2* d2 = new Derived2();
int i = null;     // error; int cannot be widened to null
Base* b = null;     // valid; null can be widened to any pointer
b = d;              // valid; Derived* can be widened to Base*
b = b2;             // error, b2 is not derived from b
b2 = d2;            // valid; Derived2 can be widened to Base2*
d2 = d;             // error; Derived2 and Derived are unrelated
return 0.0;
