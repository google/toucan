include "cube.t"
include "cubic.t"
include "event-handler.t"
include "quaternion.t"
include "transform.t"
include "teapot.t"

class Vertex {
  Vertex(float<3> p, float<3> n) { position = p; normal = n; }
  float<3> position;
  float<3> normal;
}

using Format = RGBA8unorm;

class CubeLoader {
  static void Load(Device* device, ubyte[]^ data, TextureCube<Format>^ texture, uint face) {
    auto image = new ImageDecoder<Format>(data);
    auto buffer = new Buffer<Format::MemoryType[]>(device, texture.MinBufferWidth() * image.Height());
    writeonly Format::MemoryType[]^ b = buffer.MapWrite();
    image.Decode(b, texture.MinBufferWidth());
    buffer.Unmap();
    CommandEncoder* encoder = new CommandEncoder(device);
    texture.CopyFromBuffer(encoder, buffer, image.Width(), image.Height(), 1, uint<3>(0, 0, face));
    device.GetQueue().Submit(encoder.Finish());
  }
}

Device* device = new Device();

auto texture = new sampleable TextureCube<RGBA8unorm>(device, 2176, 2176);
CubeLoader.Load(device, inline("third_party/home-cube/right.jpg"), texture, 0);
CubeLoader.Load(device, inline("third_party/home-cube/left.jpg"), texture, 1);
CubeLoader.Load(device, inline("third_party/home-cube/top.jpg"), texture, 2);
CubeLoader.Load(device, inline("third_party/home-cube/bottom.jpg"), texture, 3);
CubeLoader.Load(device, inline("third_party/home-cube/front.jpg"), texture, 4);
CubeLoader.Load(device, inline("third_party/home-cube/back.jpg"), texture, 5);

Window* window = new Window(device, 0, 0, 1024, 1024);
auto swapChain = new SwapChain<PreferredSwapChainFormat>(window);

auto cubeVB = new vertex Buffer<float<3>[]>(device, &cubeVerts);
auto cubeIB = new index Buffer<uint[]>(device, &cubeIndices);

class Tessellator {
  Tessellator(float<3>[]^ controlPoints, uint[]^ controlIndices, int level) {
    int numPatches = controlIndices.length / 16;
    int patchWidth = level + 1;  // FIXME: use wraparound and remove "+ 1"
    int verticesPerPatch = patchWidth * patchWidth;
    int numVertices = numPatches * verticesPerPatch;
    int numIndices = numPatches * level * level * 6;
    vertices = new Vertex[numVertices];
    indices = new uint[numIndices];
    int vi = 0, ii = 0;
    float scale = 1.0 / (float) level;
    for (int k = 0; k < controlIndices.length;) {
      Cubic<float<3>>[4] cubics;
      for (int i = 0; i < 4; ++i) {
        float<3>[4] p;
        for (int j = 0; j < 4; ++j) {
          p[j] = controlPoints[controlIndices[k++]];
        }
        cubics[i].FromBezier(p[0], p[1], p[2], p[3]);
      }
      for (int i = 0; i <= level; ++i) {
        float t = (float) i * scale;
        float<3>[4] p;
        for (int j = 0; j < 4; ++j) {
          p[j] = cubics[j].Evaluate(t);
        }
        Cubic<float<3>> cubic;
        cubic.FromBezier(p[0], p[1], p[2], p[3]);
        for (int j = 0; j <= level; ++j) {
          float s = (float) j * scale;
          vertices[vi].position = cubic.Evaluate(s);
          vertices[vi].normal = cubic.EvaluateTangent(s);
          if (i < level && j < level) {
            indices[ii] = vi;
            indices[ii + 1] = vi + 1;
            indices[ii + 2] = vi + patchWidth + 1;
            indices[ii + 3] = vi;
            indices[ii + 4] = vi + patchWidth + 1;
            indices[ii + 5] = vi + patchWidth;
            ii += 6;
          }
          ++vi;
        }
      }
    }
  }
  Vertex[]* vertices;
  uint[]* indices;
}

Tessellator* tessTeapot = new Tessellator(&teapotControlPoints, &teapotIndices, 8);

auto teapotVB = new vertex Buffer<Vertex[]>(device, tessTeapot.vertices);
auto teapotIB = new index Buffer<uint[]>(device, tessTeapot.indices);

class Uniforms {
  float<4,4>  model, view, projection;
//  float<4,4>  viewInverse;
}

class Bindings {
  Sampler* sampler;
  SampleableTextureCube<float>* textureView;
  uniform Buffer<Uniforms>* uniforms;
}

class SkyboxPipeline {
    float<3> vertexShader(VertexBuiltins vb, float<3> v) vertex {
        auto uniforms = bindings.uniforms.MapReadUniform();
        auto pos = float<4>(v.x, v.y, v.z, 1.0);
        vb.position = uniforms.projection * uniforms.view * uniforms.model * pos;
        return v;
    }
    float<4> fragmentShader(FragmentBuiltins fb, float<3> position) fragment {
      float<3> p = Math.normalize(position);
      // TODO: figure out why the skybox is X-flipped
      return bindings.textureView.Sample(bindings.sampler, float<3>(-p.x, p.y, p.z));
    }
    Bindings bindings;
};

class ReflectionPipeline {
    Vertex vertexShader(VertexBuiltins vb, Vertex v) vertex {
        auto n = Math.normalize(v.normal);
        auto uniforms = bindings.uniforms.MapReadUniform();
        auto viewModel = uniforms.view * uniforms.model;
        auto pos = viewModel * float<4>(v.position.x, v.position.y, v.position.z, 1.0);
        auto normal = viewModel * float<4>(n.x, n.y, n.z, 0.0);
        vb.position = uniforms.projection * pos;
        Vertex varyings;
        varyings.position = float<3>(pos.x, pos.y, pos.z);
        varyings.normal = float<3>(normal.x, normal.y, normal.z);
        return varyings;
    }
    float<4> fragmentShader(FragmentBuiltins fb, Vertex varyings) fragment {
      auto uniforms = bindings.uniforms.MapReadUniform();
      float<3> p = Math.normalize(varyings.position);
      float<3> n = Math.normalize(varyings.normal);
      float<3> r = Math.reflect(-p, n);
      auto r4 = Math.inverse(uniforms.view) * float<4>(r.x, r.y, r.z, 0.0);
      return bindings.textureView.Sample(bindings.sampler, float<3>(-r4.x, r4.y, r4.z));
    }
    Bindings bindings;
};

auto depthState = new DepthStencilState<Depth24Plus>();

auto cubePipeline = new RenderPipeline<SkyboxPipeline>(device, depthState, TriangleList);
auto cubeBindings = new Bindings();
cubeBindings.uniforms = new uniform Buffer<Uniforms>(device);
cubeBindings.sampler = new Sampler(device, ClampToEdge, ClampToEdge, ClampToEdge, Linear, Linear, Linear);
cubeBindings.textureView = texture.CreateSampleableView();
auto cubeBindGroup = new BindGroup(device, cubeBindings);

auto teapotPipeline = new RenderPipeline<ReflectionPipeline>(device, depthState, TriangleList);
auto teapotBindings = new Bindings();
teapotBindings.sampler = cubeBindings.sampler;
teapotBindings.textureView = cubeBindings.textureView;
teapotBindings.uniforms = new uniform Buffer<Uniforms>(device);
auto teapotBindGroup = new BindGroup(device, teapotBindings);

EventHandler handler;
handler.rotation = float<2>(0.0, 0.0);
handler.distance = 10.0;
float<4, 4> projection = Transform.projection(0.5, 200.0, -1.0, 1.0, -1.0, 1.0);
auto teapotQuat = Quaternion(float<3>(1.0, 0.0, 0.0), -3.1415926 / 2.0);
teapotQuat.normalize();
auto teapotRotation = teapotQuat.toMatrix();
auto depthBuffer = new renderable Texture2D<Depth24Plus>(device, 1024, 1024);
while (System.IsRunning()) {
  Quaternion orientation = Quaternion(float<3>(0.0, 1.0, 0.0), handler.rotation.x);
  orientation = orientation.mul(Quaternion(float<3>(1.0, 0.0, 0.0), handler.rotation.y));
  orientation.normalize();
  Uniforms uniforms;
  uniforms.projection = projection;
  uniforms.view = Transform.translate(0.0, 0.0, -handler.distance);
  uniforms.view *= orientation.toMatrix();
  uniforms.model = Transform.scale(100.0, 100.0, 100.0);
//  uniforms.viewInverse = Transform.invert(uniforms.view);
  cubeBindings.uniforms.SetData(&uniforms);
  uniforms.model = teapotRotation * Transform.scale(2.0, 2.0, 2.0);
  teapotBindings.uniforms.SetData(&uniforms);
  auto framebuffer = swapChain.GetCurrentTexture();
  CommandEncoder* encoder = new CommandEncoder(device);
  RenderPassEncoder* passEncoder = encoder.BeginRenderPass(framebuffer, depthBuffer);

  passEncoder.SetPipeline(cubePipeline);
  passEncoder.SetBindGroup(0, cubeBindGroup);
  passEncoder.SetVertexBuffer(0, cubeVB);
  passEncoder.SetIndexBuffer(cubeIB);
  passEncoder.DrawIndexed(cubeIndices.length, 1, 0, 0, 0);

  passEncoder.SetPipeline(teapotPipeline);
  passEncoder.SetBindGroup(0, teapotBindGroup);
  passEncoder.SetVertexBuffer(0, teapotVB);
  passEncoder.SetIndexBuffer(teapotIB);
  passEncoder.DrawIndexed(tessTeapot.indices.length, 1, 0, 0, 0);

  passEncoder.End();
  CommandBuffer* cb = encoder.Finish();
  device.GetQueue().Submit(cb);
  swapChain.Present();

  handler.Handle(System.GetNextEvent());
}
return 0.0;
