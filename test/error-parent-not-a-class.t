class A : int {
};

class Template<T> : T {
};

var a = new Template<A>;      // OK; parent is "A"
var b = new Template<float>;  // Not OK; parent is "float".
