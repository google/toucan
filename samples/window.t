Device* device = new Device();
Window* window = new Window({0, 0}, System.GetScreenSize());
var swapChain = new SwapChain<PreferredSwapChainFormat>(device, window);
var framebuffer = swapChain.GetCurrentTexture();
var encoder = new CommandEncoder(device);
class Pipeline {
  ColorAttachment<PreferredSwapChainFormat>* color;
}
var fb = new ColorAttachment<PreferredSwapChainFormat>(framebuffer, Clear, Store, float<4>(0.0, 1.0, 0.0, 1.0));
var renderPass = new RenderPass<Pipeline>(encoder, { fb });
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
