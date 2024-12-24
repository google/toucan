include "test.t"

var s = new short();

s: = 42s;

var wpi = (^int) (^short) s;

wpi: = 42;

Test.Expect((int) s: == 42);
