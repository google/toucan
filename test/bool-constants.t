bool a = true;
bool b = false;
if (a == b) {
  return 1.0;
} else if (b == a) {
  return 2.0;
} else if (a == a) {
  return 3.0;
} else {
  return 4.0;
}
return -1.0;
