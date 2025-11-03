class MipmapGeneratorPipeline {
  vertex main(vb : &VertexBuiltins) : float<2> {
    var verts = [6]float<2>{
      { -1.0, -1.0 }, { 1.0, -1.0 }, { -1.0, 1.0 },
      { -1.0,  1.0 }, { 1.0, -1.0 }, {  1.0, 1.0 }
    };
    var v = verts[vb.vertexIndex];
    vb.position = {@v, 0.0, 1.0};
    return v * float<2>{0.5, -0.5} + float<2>{0.5};
  }
}

class MipmapGenerator2DBindings {
  var sampler : *Sampler;
  var texture : *SampleableTexture2D<float>;
}

class MipmapGenerator2DPipeline<PF> : MipmapGeneratorPipeline {
  fragment main(fb : &FragmentBuiltins, texCoord : float<2>) {
    var b = bindings.Get();
    fragColor.Set(b.texture.Sample(b.sampler, texCoord));
  }
  var fragColor : *ColorOutput<PF>;
  var bindings : *BindGroup<MipmapGenerator2DBindings>;
};

class MipmapGeneratorCubeBindings {
  var sampler : *Sampler;
  var texture : *SampleableTextureCube<float>;
  var uniforms : *uniform Buffer<uint>;
}

class MipmapGeneratorCubePipeline<PF> : MipmapGeneratorPipeline {
  fragment main(fb : &FragmentBuiltins, texCoord : float<2>) {
    var faceMatrices = [6]float<3,3>{
      { { 0.0, 0.0, -2.0 }, { 0.0, -2.0,  0.0 }, {  1.0,  1.0,  1.0 } },
      { { 0.0, 0.0,  2.0 }, { 0.0, -2.0,  0.0 }, { -1.0,  1.0, -1.0 } },
      { { 2.0, 0.0,  0.0 }, { 0.0,  0.0,  2.0 }, { -1.0,  1.0, -1.0 } },
      { { 2.0, 0.0,  0.0 }, { 0.0,  0.0, -2.0 }, { -1.0, -1.0,  1.0 } },
      { { 2.0, 0.0,  0.0 }, { 0.0, -2.0,  0.0 }, { -1.0,  1.0,  1.0 } },
      { {-2.0, 0.0,  0.0 }, { 0.0, -2.0,  0.0 }, {  1.0,  1.0, -1.0 } }
    };
    var b = bindings.Get();
    var face = b.uniforms.MapRead():;
    var coord = faceMatrices[face] * float<3>{@texCoord, 1.0};
    fragColor.Set(b.texture.Sample(b.sampler, coord));
  }
  var fragColor : *ColorOutput<PF>;
  var bindings : *BindGroup<MipmapGeneratorCubeBindings>;
};

class MipmapGenerator<PF> {
  static Generate(device : *Device, texture : *renderable sampleable Texture2D<PF>) {
    var resamplingPipeline = new RenderPipeline<MipmapGenerator2DPipeline<PF>>(device);
    var mipCount = 30 - Math.clz(texture.GetSize().x); // FIXME: needs Math.max()

    var bindings : MipmapGenerator2DBindings;
    bindings.sampler = new Sampler(device);

    for (var mipLevel = 1u; mipLevel < mipCount; ++mipLevel) {
      bindings.texture = texture.CreateSampleableView(mipLevel - 1, 1u);
      var fb = texture.CreateRenderableView(mipLevel);
      var encoder = new CommandEncoder(device);
      var renderPass = new RenderPass<MipmapGenerator2DPipeline<PF>>(encoder, {
        fragColor = fb.CreateColorOutput(LoadOp.Clear),
        bindings = new BindGroup<MipmapGenerator2DBindings>(device, &bindings)
      });
      renderPass.SetPipeline(resamplingPipeline);
      renderPass.Draw(6, 1, 0, 0);
      renderPass.End();
      device.GetQueue().Submit(encoder.Finish());
    }
  }
  static Generate(device : *Device, texture : *renderable sampleable TextureCube<PF>) {
    var resamplingPipeline = new RenderPipeline<MipmapGeneratorCubePipeline<PF>>(device);
    var mipCount = 30 - Math.clz(texture.GetSize().x);

    var bindings : MipmapGeneratorCubeBindings;
    bindings.sampler = new Sampler(device);
    bindings.uniforms = new uniform Buffer<uint>(device);

    for (var face = 0u; face < 6u; ++face) {
      for (var mipLevel = 1u; mipLevel < mipCount; ++mipLevel) {
        bindings.texture = texture.CreateSampleableView(mipLevel - 1, 1u);
        bindings.uniforms.SetData(&face);
        var fb = texture.CreateRenderableView(face, mipLevel);
        var encoder = new CommandEncoder(device);
        var renderPass = new RenderPass<MipmapGeneratorCubePipeline<PF>>(encoder, {
          fragColor = fb.CreateColorOutput(LoadOp.Clear),
          bindings = new BindGroup<MipmapGeneratorCubeBindings>(device, &bindings)
        });
        renderPass.SetPipeline(resamplingPipeline);
        renderPass.Draw(6, 1, 0, 0);
        renderPass.End();
        device.GetQueue().Submit(encoder.Finish());
      }
    }
  }
}
