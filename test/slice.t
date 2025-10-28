include "include/test.t"

var array = [3]int{0, 1, 2};
var slice12 = &array[1..3];

Test.Expect(slice12.length == 2);
Test.Expect(slice12[0] == 1);
Test.Expect(slice12[1] == 2);

var slice1n = &array[1..];

Test.Expect(slice1n.length == 2);
Test.Expect(slice1n[0] == 1);
Test.Expect(slice1n[1] == 2);

Test.Expect(array[..].length == 3);
Test.Expect(array[..1].length == 1);
Test.Expect(array[..2].length == 2);
Test.Expect(array[..3].length == 3);

Test.Expect(array[0..0].length == 0);
Test.Expect(array[0..1].length == 1);
Test.Expect(array[0..2].length == 2);
Test.Expect(array[0..3].length == 3);
Test.Expect(array[0..].length == 3);
