include "include/dragon.t"
include "include/mesh.t"
include "include/transform.t"
include "include/tex-coord-utils.t"

class LightData {
  var position : float<4>;
  var color    : float<3>;
  var radius   : float;
}

class Config {
  var numLights : uint;
}

class LightExtent {
  var min : float<3>;
  var max : float<3>;
}

class LightUpdateBindings {
  var lights : *storage Buffer<[]LightData>;
  var config : *uniform Buffer<Config>;
  var lightExtent : *uniform Buffer<LightExtent>;
}

class LightUpdate {
  compute(64, 1, 1) main(cb : &ComputeBuiltins) {
    var lights = bindings.Get().lights.Map();
    var config = bindings.Get().config.MapRead();
    var lightExtent = bindings.Get().lightExtent.MapRead();
    var i = cb.globalInvocationId.x;

    if (i >= config.numLights) {
      return;
    }

    lights[i].position.y = lights[i].position.y - 0.5 - 0.003 * ((float)(i) - 64.0 * Math.floor((float)(i) / 64.0));

    if (lights[i].position.y < lightExtent.min.y) {
      lights[i].position.y = lightExtent.max.y;
    }
  }

  var bindings : *BindGroup<LightUpdateBindings>;
}

class Uniforms {
  var modelMatrix : float<4,4>;
  var normalModelMatrix : float<4,4>;
}

class Camera {
  var viewProjectionMatrix : float<4,4>;
  var invViewProjectionMatrix : float<4,4>;
}

class VertexOutput {
  var fragNormal : float<3>;
  var fragUV : float<2>;
}

class WriteGBuffersBindings {
  var uniforms : *uniform Buffer<Uniforms>;
  var camera : *uniform Buffer<Camera>;
}

class Vertex {
  var position : float<3>;
  var normal : float<3>;
  var uv : float<2>;
}

class WriteGBuffers {
  vertex main(vb : &VertexBuiltins) : VertexOutput {
    var uniforms = bindings.Get().uniforms.MapRead();
    var camera = bindings.Get().camera.MapRead();
    var v = vertexes.Get();

    // Transform the vertex position by the model and viewProjection matrices.
    // Transform the vertex normal by the normalModelMatrix (inverse transpose of the model).
    var output : VertexOutput;
    var worldPosition = uniforms.modelMatrix * float<4>{@v.position, 1.0};
    vb.position = camera.viewProjectionMatrix * float<4>{@worldPosition.xyz, 1.0};
    var fragNormal = uniforms.normalModelMatrix * float<4>{@v.normal, 1.0};
    output.fragNormal = fragNormal.xyz;
    output.fragUV = v.uv;
    return output;
  }

  fragment main(fb : &FragmentBuiltins, varyings : VertexOutput) {
    var uv = Math.floor(30.0 * varyings.fragUV);
    var c = 0.2 + 0.5 * ((uv.x + uv.y) - 2.0 * Math.floor((uv.x + uv.y) / 2.0));

    normals.Set({@Math.normalize(varyings.fragNormal), 1.0});
    albedo.Set({c, c, c, 1.0});
  }

  var normals : *ColorOutput<RGBA16float>;
  var albedo : *ColorOutput<BGRA8unorm>;
  var depth : *DepthStencilOutput<Depth24Plus>;
  var bindings : *BindGroup<WriteGBuffersBindings>;
  var vertexes : *VertexInput<Vertex>;
  var indexes : *index Buffer<[]ushort>;
}

class TextureQuadPass {
  vertex main(vb : &VertexBuiltins) {
    var pos : [6]float<2> = {
      { -1.0, -1.0 }, { 1.0, -1.0 }, { -1.0,  1.0 },
      { -1.0,  1.0 }, { 1.0, -1.0 }, {  1.0,  1.0 } };
    vb.position = {@pos[vb.vertexIndex], 0.0, 1.0};
  }

  var fragColor : *ColorOutput<PreferredPixelFormat>;
}

class GBufferTextureBindings {
  var gBufferNormal : *SampleableTexture2D<float>;
  var gBufferAlbedo : *SampleableTexture2D<float>;
  var gBufferDepth : *unfilterable SampleableTexture2D<float>;
}

class WindowSizeBindings {
  var size : *uniform Buffer<uint<2>>;
}

class GBuffersDebugView : TextureQuadPass {
  fragment main(fb : &FragmentBuiltins) {
    var gBufferDepth = textureBindings.Get().gBufferDepth;
    var gBufferNormal = textureBindings.Get().gBufferNormal;
    var gBufferAlbedo = textureBindings.Get().gBufferAlbedo;
    var result : float<4>;
    var windowSize = windowSizeBindings.Get().size.MapRead():;
    var c = fb.fragCoord.xy / (float<2>) windowSize;
    if (c.x < 0.33333) {
      var rawDepth = gBufferDepth.Load((uint<2>) Math.floor(fb.fragCoord.xy), 0).x;
      // Remap depth into something a bit more visible.
      var depth = (1.0 - rawDepth) * 50.0;
      result = float<4>(depth);
    } else if (c.x < 0.66667) {
      result = gBufferNormal.Load((uint<2>) Math.floor(fb.fragCoord.xy), 0);
      result.x = (result.x + 1.0) * 0.5;
      result.y = (result.y + 1.0) * 0.5;
      result.z = (result.z + 1.0) * 0.5;
    } else {
      result = gBufferAlbedo.Load((uint<2>) Math.floor(fb.fragCoord.xy), 0);
    }
    fragColor.Set(result);
  }

  var textureBindings : *BindGroup<GBufferTextureBindings>;
  var windowSizeBindings : *BindGroup<WindowSizeBindings>;
}

class DeferredRenderBufferBindings {
  var lights : *storage Buffer<[]LightData>;
  var config : *uniform Buffer<Config>;
  var camera : *uniform Buffer<Camera>;
}

class DeferredRender : TextureQuadPass {
  deviceonly static worldFromScreenCoord(camera : Camera, coord : float<2>, depthSample : float) : float<3> {
    // Reconstruct world-space position from the screen coordinates.
    var posClip = float<4>(coord.x * 2.0 - 1.0, (1.0 - coord.y) * 2.0 - 1.0, depthSample, 1.0);
    var posWorldW = camera.invViewProjectionMatrix * posClip;
    var posWorld = posWorldW.xyz / posWorldW.www;
    return posWorld;
  }

  fragment main(fb : &FragmentBuiltins) {
    var buffers = bufferBindings.Get();
    var config = buffers.config.MapRead();
    var lights = buffers.lights.Map();
    var camera = buffers.camera.MapRead():;
    var textures = textureBindings.Get();
    var result : float<3>;

    var depth = textures.gBufferDepth.Load((uint<2>) Math.floor(fb.fragCoord.xy), 0).x;

    // Don't light the sky.
    if (depth >= 1.0) {
      return;
    }

    var bufferSize = textures.gBufferDepth.GetSize();
    var coordUV = fb.fragCoord.xy / (float<2>) bufferSize;
    var position = this.worldFromScreenCoord(camera, coordUV, depth);
    var normal = textures.gBufferNormal.Load((uint<2>) Math.floor(fb.fragCoord.xy), 0).xyz;
    var albedo = textures.gBufferAlbedo.Load((uint<2>) Math.floor(fb.fragCoord.xy), 0).xyz;

    for (var i = 0; i < config.numLights; i++) {
      var L = lights[i].position.xyz - position;
      var distance = Math.length(L);
      if (distance <= lights[i].radius) {
        var lambert = Math.max(Math.dot(normal, Math.normalize(L)), 0.0);
        result += lambert * Math.pow(1.0 - distance / lights[i].radius, 2.0) * lights[i].color * albedo;
      }
    }

    // Add some manual ambient.
    result += float<3>(0.2);

    fragColor.Set({@result, 1.0});
  }

  var textureBindings : *BindGroup<GBufferTextureBindings>;
  var bufferBindings : *BindGroup<DeferredRenderBufferBindings>;
}

var kMaxNumLights = 1024;
var lightExtentMin = float<3>{-50.0, -30.0, -50.0};
var lightExtentMax = float<3>{ 50.0, 50.0, 50.0};

var device = new Device();
var window = new Window(System.GetScreenSize());

var swapChain = new SwapChain<PreferredPixelFormat>(device, window);
var windowSize = window.GetSize();
var aspect = (float) windowSize.x / (float) windowSize.y;

var mesh = new Mesh<Vertex, ushort>(&dragonVertices, &dragonTriangles, Math.pi);
TexCoordUtils<Vertex>.ComputeProjectedPlaneUVs(mesh.vertices, ProjectedPlane.XY);

var groundPlaneVertices : [4]Vertex = {
  { position = { -100.0, 20.0, -100.0 }, normal = { 0.0, 1.0, 0.0 }, uv = { 0.0, 0.0 } },
  { position = {  100.0, 20.0,  100.0 }, normal = { 0.0, 1.0, 0.0 }, uv = { 1.0, 1.0 } },
  { position = { -100.0, 20.0,  100.0 }, normal = { 0.0, 1.0, 0.0 }, uv = { 0.0, 1.0 } },
  { position = {  100.0, 20.0, -100.0 }, normal = { 0.0, 1.0, 0.0 }, uv = { 1.0, 0.0 } }
};

var groundPlaneIndexes : [6]ushort = { 0us, 2us, 1us, 0us, 1us, 3us };

var groundPlaneVertexBuffer = new vertex Buffer<[]Vertex>(device, &groundPlaneVertices);
var groundPlaneIndexBuffer = new index Buffer<[]ushort>(device, &groundPlaneIndexes);

var vertexBuffer = new vertex Buffer<[]Vertex>(device, mesh.vertices);
var indexBuffer = new index Buffer<[]ushort>(device, mesh.indices);
var gBufferTexture2DFloat16 = new renderable sampleable Texture2D<RGBA16float>(device, windowSize);
var gBufferTextureAlbedo = new renderable sampleable Texture2D<BGRA8unorm>(device, windowSize);
var depthTexture = new renderable sampleable Texture2D<Depth24Plus>(device, windowSize);

var writeGBuffersPipeline = new RenderPipeline<WriteGBuffers>(
  device = device,
  cullMode = CullMode.Back
);

var gBuffersDebugViewPipeline = new RenderPipeline<GBuffersDebugView>(device);
var deferredRenderPipeline = new RenderPipeline<DeferredRender>(device);

var writeGBufferPassDescriptor = WriteGBuffers{
  normals = gBufferTexture2DFloat16.CreateColorOutput(
    clearValue = {0.0, 0.0, 0.0, 1.0},
    loadOp = LoadOp.Clear
  ),
  albedo = gBufferTextureAlbedo.CreateColorOutput(
    clearValue = {0.0, 0.0, 0.0, 1.0},
    loadOp = LoadOp.Clear
  ),
  depth = depthTexture.CreateDepthStencilOutput(
    depthLoadOp = LoadOp.Clear,
    depthClearValue = 1.0
  )
};

enum Mode {
  Rendering,
  GBuffersView
}

class Settings {
  var mode = Mode.Rendering;
  var numLights = 128;
}

var settings : Settings;

var configUniformBuffer = new uniform Buffer<Config>(device, {settings.numLights});
var modelUniformBuffer = new uniform Buffer<Uniforms>(device);
var cameraUniformBuffer = new uniform Buffer<Camera>(device);
var sceneUniformBindGroup = new BindGroup<WriteGBuffersBindings>(device, {
  uniforms = modelUniformBuffer,
  camera = cameraUniformBuffer
});

var gBufferTexturesBindGroup = new BindGroup<GBufferTextureBindings>(device, {
  gBufferNormal = gBufferTexture2DFloat16.CreateSampleableView(),
  gBufferAlbedo = gBufferTextureAlbedo.CreateSampleableView(),
  gBufferDepth = depthTexture.CreateSampleableView()
});

// Lights data are uploaded in a storage buffer
// which could be updated/culled/etc. with a compute shader.
var extent = lightExtentMax - lightExtentMin;
var lightsBuffer = new storage Buffer<[]LightData>(device, kMaxNumLights);

// We randomaly populate lights randomly in a box range
// and simply move them along y-axis per frame to show they are
// dynamic lighting.
var lightData = [kMaxNumLights] new LightData;
for (var j = 0; j < kMaxNumLights; j++) {
  var light = &lightData[j];
  for (var i = 0; i < 3; i++) {
    light.position[i] = Math.rand() * extent[i] + lightExtentMin[i];
  }
  light.position[3] = 1.0;
  light.color = {
    Math.rand() * 2.0,
    Math.rand() * 2.0,
    Math.rand() * 2.0
  };
  light.radius = 20.0;
}
lightsBuffer.SetData(lightData);

var lightExtentBuffer = new uniform Buffer<LightExtent>(device, {lightExtentMin, lightExtentMax});
var lightUpdateComputePipeline = new ComputePipeline<LightUpdate>(device);
var lightsBufferBindGroup = new BindGroup<DeferredRenderBufferBindings>(device, {
  lightsBuffer,
  configUniformBuffer,
  cameraUniformBuffer
});

var lightsBufferComputeBindGroup = new BindGroup<LightUpdateBindings>(device, {
  lightsBuffer,
  configUniformBuffer,
  lightExtentBuffer
});

var eyePosition = float<3>(0.0, 50.0, -100.0);
var upVector = float<3>(0.0, 1.0, 0.0);
var origin = float<3>(0.0, 0.0, 0.0);

var projectionMatrix = Transform.perspective((2.0 * Math.pi) / 5.0, aspect, 1.0, 2000.0);

// Move the model so it's centered.
var modelMatrix = Transform.translation({0.0, -45.0, 0.0});

// Compute the inverse transpose for transforming normals.
var invertTransposeModelMatrix = Transform.invert(modelMatrix);
invertTransposeModelMatrix = Math.transpose(invertTransposeModelMatrix);

// Set the matrix uniform data.
modelUniformBuffer.SetData({modelMatrix, invertTransposeModelMatrix});

var startTime = System.GetCurrentTime();
while (System.IsRunning()) {
  // Rotate the camera around the origin based on time.
  var rad = Math.pi * (float) ((System.GetCurrentTime() - startTime) / 5.0d);
  var rotation = Transform.translation(origin) * Transform.rotation({0.0, 1.0, 0.0}, rad);
  var rp4 = rotation * float<4>{@eyePosition, 1.0};
  rp4 /= rp4.w;
  var rotatedEyePosition = rp4.xyz;

  var viewMatrix = Transform.lookAt(rotatedEyePosition, origin, upVector);

  // Update camera matrices.
  var camera : Camera;
  camera.viewProjectionMatrix = projectionMatrix * viewMatrix;
  camera.invViewProjectionMatrix = Transform.invert(camera.viewProjectionMatrix);
  cameraUniformBuffer.SetData(&camera);

  var commandEncoder = new CommandEncoder(device);
  {
  // Write position, normal, albedo etc. data to gBuffers.
    var gBufferPass = new RenderPass<WriteGBuffers>(commandEncoder, &writeGBufferPassDescriptor);
    gBufferPass.SetPipeline(writeGBuffersPipeline);
    gBufferPass.Set({bindings = sceneUniformBindGroup,
                     vertexes = new VertexInput<Vertex>(vertexBuffer),
                     indexes = indexBuffer});
    gBufferPass.DrawIndexed(mesh.indices.length, 1, 0, 0, 0);
    gBufferPass.Set({vertexes = new VertexInput<Vertex>(groundPlaneVertexBuffer),
                     indexes = groundPlaneIndexBuffer});
    gBufferPass.DrawIndexed(groundPlaneIndexes.length, 1, 0, 0, 0);
    gBufferPass.End();
  }
  {
    // Update light positions.
    var lightPass = new ComputePass<LightUpdate>(commandEncoder, {});
    lightPass.SetPipeline(lightUpdateComputePipeline);
    lightPass.Set({bindings = lightsBufferComputeBindGroup});
    lightPass.Dispatch((kMaxNumLights + 63) / 64, 1, 1);
    lightPass.End();
  }
  if (settings.mode == Mode.GBuffersView) {
    // GBuffers debug view
    // Left: depth
    // Middle: normal
    // Right: albedo (use uv to mimic a checkerboard texture)
    var fb = swapChain
      .GetCurrentTexture()
      .CreateColorOutput(LoadOp.Clear, StoreOp.Store, {0.0, 0.0, 1.0, 1.0});
    var debugViewPass = new RenderPass<GBuffersDebugView>(commandEncoder, {
      fragColor = fb
    });
    var windowSizeBuffer = new uniform Buffer<uint<2>>(device, &windowSize);
    var windowSizeBindGroup = new BindGroup<WindowSizeBindings>(device, {windowSizeBuffer});
    debugViewPass.SetPipeline(gBuffersDebugViewPipeline);
    debugViewPass.Set({textureBindings = gBufferTexturesBindGroup,
                       windowSizeBindings = windowSizeBindGroup});
    debugViewPass.Draw(6, 1, 0, 0);
    debugViewPass.End();
  } else {
    // Deferred rendering.
    var fb = swapChain
      .GetCurrentTexture()
      .CreateColorOutput(LoadOp.Clear, StoreOp.Store, {0.0, 0.0, 1.0, 1.0});
    var deferredRenderingPass = new RenderPass<DeferredRender>(commandEncoder, {
      fragColor = fb
    });
    deferredRenderingPass.SetPipeline(deferredRenderPipeline);
    deferredRenderingPass.Set({textureBindings = gBufferTexturesBindGroup,
                               bufferBindings = lightsBufferBindGroup});
    deferredRenderingPass.Draw(6, 1, 0, 0);
    deferredRenderingPass.End();
  }
  device.GetQueue().Submit(commandEncoder.Finish());
  swapChain.Present();
  while (System.HasPendingEvents()) {
    System.GetNextEvent();
  }
}
