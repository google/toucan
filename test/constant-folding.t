include "include/test.t"

var int4Array : [128]int<4> = { -int<4>{ -42, -21, -14, -7 } };
Test.Expect(int4Array[0].w == 7);
Test.Expect(int4Array[31].z == 14);
Test.Expect(int4Array[64].y == 21);
Test.Expect(int4Array[127].x == 42);

var shortArray : [512]short = { -42s };
Test.Expect(shortArray[0] as int == -42);
Test.Expect(shortArray[31] as int == -42);
Test.Expect(shortArray[64] as int == -42);
Test.Expect(shortArray[127] as int == -42);

var ubyteArray : [1024]ubyte = { -42ub };
Test.Expect(ubyteArray[0] as int == -42);
Test.Expect(ubyteArray[511] as int == -42);
Test.Expect(ubyteArray[768] as int == -42);
Test.Expect(ubyteArray[1023] as int == -42);

var byteArray : [1024]ubyte = { -42b };
Test.Expect(byteArray[0] as int == -42);
Test.Expect(byteArray[511] as int == -42);
Test.Expect(byteArray[768] as int == -42);
Test.Expect(byteArray[1023] as int == -42);

var boolArray : [1024]bool = { true };
Test.Expect(boolArray[0] == true);
Test.Expect(boolArray[511] == true);
Test.Expect(boolArray[768] == true);
Test.Expect(boolArray[1023] == true);
