class EventHandler {
  void Handle(Event* event) {
    if (event.type == MouseDown) {
      active = true;
    } else if (event.type == MouseUp) {
      active = false;
    } else if (event.type == MouseMove) {
      int<2> diff = event.position - anchor;
      if (active || (event.modifiers & Control) != 0) {
        theta += (float) diff.x / 200.0;
        phi += (float) diff.y / 200.0;
      } else if ((event.modifiers & Shift) != 0) {
        distance += (float) diff.y / 100.0;
      }
      anchor = event.position;
    }
  }
  float theta, phi, distance;
  int<2> anchor;
  bool active = false;
}
