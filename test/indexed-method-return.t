include "include/test.t"

class Foo {
  static generateArray() : *[]float {
    var r = [10] new float;
    r[9] = 1234.0;
    return r;
  }
  static generateFixedArray() : [3]int {
    return [3]int(3, 2, 1);
  }
  static generateVector() : float<2> {
    return float<2>{42.0, 21.0};
  }
  static generateMatrix() : float<2, 2> {
    return float<2, 2>{float<2>{4.0, 3.0}, float<2>{2.0, 1.0}};
  }
}

Test.Expect(Foo.generateArray()[9] == 1234.0);
Test.Expect(Foo.generateFixedArray()[0] == 3);
Test.Expect(Foo.generateFixedArray()[1] == 2);
Test.Expect(Foo.generateFixedArray()[2] == 1);
Test.Expect(Foo.generateVector()[0] == 42.0);
Test.Expect(Foo.generateVector()[1] == 21.0);
Test.Expect(Foo.generateMatrix()[0][0] == 4.0);
Test.Expect(Foo.generateMatrix()[0][1] == 3.0);
Test.Expect(Foo.generateMatrix()[1][0] == 2.0);
Test.Expect(Foo.generateMatrix()[1][1] == 1.0);
