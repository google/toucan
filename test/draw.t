Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);
auto framebuffer = swapChain.GetCurrentTexture();
auto encoder = new CommandEncoder(device);
auto renderPass = new RenderPass(encoder, framebuffer, null, 1.0, 0.0, 0.0, 1.0);
renderPass.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
return 0.0;
