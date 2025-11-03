class CubeLoader<PF> {
  Load(data : *[]ubyte, face : uint) {
    var image = new Image<PF>(data);
    var size = image.GetSize();
    var buffer = new hostwriteable Buffer<[]ubyte<4>>(device, texture.MinBufferWidth() * size.y);
    image.Decode(buffer.MapWrite(), texture.MinBufferWidth());
    var encoder = new CommandEncoder(device);
    texture.CopyFromBuffer(encoder, buffer, {@size, 1}, {0, 0, face});
    device.GetQueue().Submit(encoder.Finish());
  }
  var device: *Device;
  var texture: *TextureCube<PF>;
}
