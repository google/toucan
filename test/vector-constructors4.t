include "include/test.t"

float<4> v41 = float<4>(1.0);
float<4> v42 = float<4>(2.0, 3.0);
float<4> v43 = float<4>(4.0, 5.0, 6.0);
float<4> v44 = float<4>(7.0, 8.0, 9.0, 10.0);

Test.Expect(v41.x == 1.0);
Test.Expect(v41.y == 1.0);
Test.Expect(v41.z == 1.0);
Test.Expect(v41.w == 1.0);
Test.Expect(v42.x == 2.0);
Test.Expect(v42.y == 3.0);
Test.Expect(v42.z == 0.0);
Test.Expect(v42.w == 0.0);
Test.Expect(v43.x == 4.0);
Test.Expect(v43.y == 5.0);
Test.Expect(v43.z == 6.0);
Test.Expect(v43.w == 0.0);
Test.Expect(v44.x == 7.0);
Test.Expect(v44.y == 8.0); 
Test.Expect(v44.z == 9.0); 
Test.Expect(v44.w == 10.0); 
