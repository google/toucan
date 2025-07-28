var device = new Device();
var window = new Window(System.GetScreenSize());
var swapChain = new SwapChain<PreferredPixelFormat>(device, window);
var framebuffer = swapChain.GetCurrentTexture();
var encoder = new CommandEncoder(device);
class Pipeline {
  var color : *ColorOutput<PreferredPixelFormat>;
}
var fb = framebuffer.CreateColorOutput(LoadOp.Clear, StoreOp.Store, float<4>(0.0, 1.0, 0.0, 1.0));
var renderPass = new RenderPass<Pipeline>(encoder, { fb });
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
