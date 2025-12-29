class EventHandler {
  Rotate(diff : int<2>) {
    rotation += diff as float<2> / 200.0;
  }
  Handle(event : &Event) {
    if (event.type == EventType.MouseDown) {
      mouseDown = true;
    } else if (event.type == EventType.MouseUp) {
      mouseDown = false;
    } else if (event.type == EventType.MouseMove) {
      var diff = event.position - prevPosition;
      if (mouseDown || (event.modifiers & EventModifiers.Control) != 0) {
        this.Rotate(diff);
      } else if ((event.modifiers & EventModifiers.Shift) != 0) {
        distance += diff.y as float / 100.0;
      }
      prevPosition = event.position;
    } else if (event.type == EventType.TouchStart) {
      prevTouches = event.touches;
    } else if (event.type == EventType.TouchMove) {
      if (event.numTouches != prevNumTouches) {
      } else if (event.numTouches == 1) {
        this.Rotate(event.touches[0] - prevTouches[0]);
      } else if (event.numTouches == 2) {
        var prevDistance = Math.length((prevTouches[1] - prevTouches[0]) as float<2>);
        var curDistance = Math.length((event.touches[1] - event.touches[0]) as float<2>);
        distance *= prevDistance / curDistance;
      }
      prevTouches = event.touches;
      prevNumTouches = event.numTouches;
    }
  }
  var mouseDown : bool;
  var prevPosition : int<2>;
  var prevTouches : [10]int<2>;
  var prevNumTouches : int;
  var rotation : float<2>;
  var distance : float;
}
