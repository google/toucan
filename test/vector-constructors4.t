float<4> v41 = float<4>(1.0);
float<4> v42 = float<4>(2.0, 3.0);
float<4> v43 = float<4>(4.0, 5.0, 6.0);
float<4> v44 = float<4>(7.0, 8.0, 9.0, 10.0);

return v41.x + v41.y + v41.z + v41.w
     + v42.x + v42.y + v42.z + v42.w
     + v43.x + v43.y + v43.z + v43.w
     + v44.x + v44.y + v44.z + v44.w;
