class Vertex {
    float<4> position;
    float<2> texCoord;
};
class Varyings {
    float<2> texCoord;
};
class Uniforms {
    float<4, 4> modelView;
}

class Uploader {
    void upload(Buffer buffer, int size) {
        Vertex[]* vertices = new Vertex[size];
        for (int i = 0; i < size; ++i) {
            vertices[i].position = float<4>(1.0, 2.0, 3.0, 4.0);
            vertices[i].texCoord = float<2>(.1, .2);
        }
    }
}

class ComplexShader {
    Varyings vertexShader(VertexBuiltins vb, Vertex v) vertex {
        vb.position = uniforms.MapReadUniform().modelView * v.position;
        Varyings varyings;
        varyings.texCoord = v.texCoord;
        return varyings;
    }
    float<4> fragmentShader(FragmentBuiltins fb, Varyings varyings) fragment {
      return float<4>(0.0, 1.0, 0.0, 1.0);
    }
    Sampler* sampler;
    SampleableTexture2D<float>* textureView;
    uniform Buffer<Uniforms>* uniforms;
};

auto device = new Device();
auto pipeline = new RenderPipeline<ComplexShader>(device, null, TriangleList);
return 0.0;
