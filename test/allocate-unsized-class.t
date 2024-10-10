include "include/test.t"

class C {
  var f : float;
  var array : int[];
};
var c = new [5]C();
c.array[2] = 42;

Test.Expect(c.array[2] == 42);
