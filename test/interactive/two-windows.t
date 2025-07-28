class Pipeline {
  var fragColor : *ColorOutput<PreferredPixelFormat>;
}
var green = float<4>(0.0, 1.0, 0.0, 1.0);
var blue = float<4>(0.0, 0.0, 1.0, 1.0);
var device = new Device();
var queue = device.GetQueue();
var window1 = new Window({640, 480}, {0, 0});
var window2 = new Window({640, 480}, {100, 100});
var swapChain1 = new SwapChain<PreferredPixelFormat>(device, window1);
var swapChain2 = new SwapChain<PreferredPixelFormat>(device, window2);
var framebuffer1 = swapChain1.GetCurrentTexture();
var framebuffer2 = swapChain2.GetCurrentTexture();
var encoder = new CommandEncoder(device);
var p1 = Pipeline{ framebuffer1.CreateColorOutput(LoadOp.Clear, StoreOp.Store, green) };
var renderPass1 = new RenderPass<Pipeline>(encoder, &p1);
renderPass1.End();
var p2 = Pipeline{ framebuffer2.CreateColorOutput(LoadOp.Clear, StoreOp.Store, blue) };
var renderPass2 = new RenderPass<Pipeline>(encoder, &p2);
renderPass2.End();
device.GetQueue().Submit(encoder.Finish());
swapChain1.Present();
swapChain2.Present();

while (System.IsRunning()) System.GetNextEvent();
