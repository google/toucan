class CubeLoader {
  static Load(device : *Device, data : *[]ubyte, texture : &TextureCube<RGBA8unorm>, face : uint) {
    var image = new Image<RGBA8unorm>(data);
    var size = image.GetSize();
    var buffer = new hostwriteable Buffer<[]ubyte<4>>(device, texture.MinBufferWidth() * size.y);
    image.Decode(buffer.MapWrite(), texture.MinBufferWidth());
    var encoder = new CommandEncoder(device);
    texture.CopyFromBuffer(encoder, buffer, {@size, 1}, {0, 0, face});
    device.GetQueue().Submit(encoder.Finish());
  }
}
