Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);
auto framebuffer = swapChain.GetCurrentTexture();
auto encoder = new CommandEncoder(device);
class Pipeline {
  ColorAttachment<PreferredSwapChainFormat>* color;
}
Pipeline p;
p.color = new ColorAttachment<PreferredSwapChainFormat>(framebuffer, Clear, Store, float<4>(0.0, 1.0, 0.0, 1.0));
auto renderPass = new RenderPass<Pipeline>(encoder, &p);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
return 0.0;
