include "test.t"

var s = new short();

s: = 42s;

var wpi = s as ^short as ^int;

wpi: = 42;

Test.Expect(s: as int == 42);
