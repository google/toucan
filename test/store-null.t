class P {}
class Q {
  Q(P* p) {
    mP = p;
  }
  P* mP;
}
P* p = new P();
Q* q = new Q(null);
