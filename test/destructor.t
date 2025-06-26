include "include/test.t"

class Bar {
  Bar(b: *bool) {
    destroyed = b;
  }
 ~Bar() {
    destroyed: = true;
  }
  var destroyed: *bool;
};

var result = new bool;
{
  var f = new Bar(result);
}
Test.Expect(result: == true);

result: = false;
{
  var f = Bar(result);
}
Test.Expect(result: == true);
