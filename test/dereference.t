auto f = new float();
*f = 3.0;
auto v = new float<3>();
*v = float<3>(2.0, 4.0, 6.0);
float<3> temp = *v;
auto a = new float[3]();
*a = float[3](3.0, 2.0, 1.0);
float[3] atemp = *a;
return *f + temp.x + atemp[1];
