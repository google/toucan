var green = float<4>(0.0, 1.0, 0.0, 1.0);
var blue = float<4>(0.0, 0.0, 1.0, 1.0);
Device* device = new Device();
Queue* queue = device.GetQueue();
Window* window1 = new Window({0, 0}, {640, 480});
Window* window2 = new Window({100, 100}, {640, 480});
var swapChain1 = new SwapChain<PreferredSwapChainFormat>(device, window1);
var swapChain2 = new SwapChain<PreferredSwapChainFormat>(device, window2);
var framebuffer1 = swapChain1.GetCurrentTexture();
var framebuffer2 = swapChain2.GetCurrentTexture();
class Pipeline {
  ColorAttachment<PreferredSwapChainFormat>* fragColor;
}
var encoder = new CommandEncoder(device);
Pipeline p1;
p1.fragColor = new ColorAttachment<PreferredSwapChainFormat>(framebuffer1, Clear, Store, green);
var renderPass1 = new RenderPass<Pipeline>(encoder, &p1);
renderPass1.End();
Pipeline p2;
p2.fragColor = new ColorAttachment<PreferredSwapChainFormat>(framebuffer2, Clear, Store, blue);
var renderPass2 = new RenderPass<Pipeline>(encoder, &p2);
renderPass2.End();
device.GetQueue().Submit(encoder.Finish());
swapChain1.Present();
swapChain2.Present();

while (System.IsRunning()) System.GetNextEvent();
