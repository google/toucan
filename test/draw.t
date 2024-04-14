Device* device = new Device();
Queue* queue = device.GetQueue();
Window* window = new Window(device, 0, 0, 640, 480);
SwapChain* swapChain = new SwapChain(window);
renderable SampleableTexture2D* framebuffer = swapChain.GetCurrentTextureView();
CommandEncoder* encoder = new CommandEncoder(device);
RenderPassEncoder* passEncoder = encoder.BeginRenderPass(framebuffer, null, 1.0, 0.0, 0.0, 1.0);
passEncoder.End();
CommandBuffer* cb = encoder.Finish();
queue.Submit(cb);
swapChain.Present();

while (System.IsRunning()) System.GetNextEvent();
return 0.0;
