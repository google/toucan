include "test.t"

var i = new int();

i: = 0;

var wpi : ^int = i;
var rpi : &int = wpi;

i: = 42;

Test.Expect(i: == 42);
Test.Expect(wpi: == 42);
Test.Expect(rpi == 42);
