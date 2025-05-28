include "include/test.t"

const c1 = 42;
Test.Assert(c1 == 42);

const c2 = 42.0;
Test.Assert(c2 == 42.0);

const c3 = 84.0d;
Test.Assert(c3 == 84.0d);

const c4 = 42b;
Test.Assert((int) c4 == 42);

const c5 = 42ub;
Test.Assert((uint) c5 == 42u);

const c6 = 42s;
Test.Assert((int) c6 == 42);

const c7 = 42us;
Test.Assert((uint) c7 == 42u);

const c8 = false;
Test.Assert(c8 == false);

const c9 = true;
Test.Assert(c9);

const c10 = false;
Test.Assert(!c10);

const c11 = float<2>{c2, 21.0};
Test.Assert(c11.x == 42.0 && c11.y == 21.0);

const c12 = float<2, 2>{{c2, 21.0}, {14.0, c2}};
Test.Assert(c12[0][0] == 42.0 && c12[0][1] == 21.0 && c12[1][0] == 14.0 && c12[1][1] == 42.0);

const scoped = 42;
{
  const scoped = 21;
  Test.Assert(scoped == 21);
}
Test.Assert(scoped == 42);
