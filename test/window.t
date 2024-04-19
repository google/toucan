Device* device = new Device();
Window* window = new Window(device, 0, 0, 640, 480);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);
auto framebuffer = swapChain.GetCurrentTexture();
CommandEncoder* encoder = new CommandEncoder(device);
RenderPassEncoder* passEncoder = encoder.BeginRenderPass(framebuffer, null, 0.0, 1.0, 0.0, 1.0);
passEncoder.End();
device.GetQueue().Submit(encoder.Finish());
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
return 0.0;
