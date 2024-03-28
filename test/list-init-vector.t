float<2> v21 = {1.0, 0.0};
float<2> v22 = {2.0, 3.0};

int<4> v = {1, 2, 3, 4};
v = {1, 1, 1, 1};

return v21.x + v21.y + v22.x + v22.y + (float)(v.x + v.y + v.z + v.w);
