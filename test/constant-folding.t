include "include/test.t"

var int4Array : [128]int<4> = { -int<4>{ -42, -21, -14, -7 } };
Test.Expect(int4Array[0].w == 7);
Test.Expect(int4Array[31].z == 14);
Test.Expect(int4Array[64].y == 21);
Test.Expect(int4Array[127].x == 42);

var shortArray : [512]short = { -42s };
Test.Expect((int) shortArray[0] == -42);
Test.Expect((int) shortArray[31] == -42);
Test.Expect((int) shortArray[64] == -42);
Test.Expect((int) shortArray[127] == -42);

var ubyteArray : [1024]ubyte = { -42ub };
Test.Expect((int) ubyteArray[0] == -42);
Test.Expect((int) ubyteArray[511] == -42);
Test.Expect((int) ubyteArray[768] == -42);
Test.Expect((int) ubyteArray[1023] == -42);

var byteArray : [1024]ubyte = { -42b };
Test.Expect((int) byteArray[0] == -42);
Test.Expect((int) byteArray[511] == -42);
Test.Expect((int) byteArray[768] == -42);
Test.Expect((int) byteArray[1023] == -42);

var boolArray : [1024]bool = { true };
Test.Expect(boolArray[0] == true);
Test.Expect(boolArray[511] == true);
Test.Expect(boolArray[768] == true);
Test.Expect(boolArray[1023] == true);
