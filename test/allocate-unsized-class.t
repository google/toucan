include "include/test.t"

class C {
  var f : float;
  var array : []int;
};
var c = [5] new C();
c.array[2] = 42;

Test.Expect(c.array[2] == 42);
