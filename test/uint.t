uint a = 3;
uint b = 3000000000u;
if (a > b) {
    return 0.0;
}
int c = 3;
int d = (int) 3000000000u;
if (c < d) {
    return 0.0;
}
return 1.0;
