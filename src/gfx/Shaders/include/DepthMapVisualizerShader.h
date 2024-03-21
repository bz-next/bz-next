#ifndef DEPTHMAPVISUALIZER_SHADER_H
#define DEPTHMAPVISUALIZER_SHADER_H

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Attribute.h>
#include <Magnum/Math/Color.h>
#include <Magnum/GL/Texture.h>

// Simple shader to render depth map to a quad for visualization
class DepthMapVisualizerShader : public Magnum::GL::AbstractShaderProgram {
    public:
    typedef Magnum::GL::Attribute<0, Magnum::Vector3> Position;
    typedef Magnum::GL::Attribute<1, Magnum::Vector2> TextureCoordinates;

    explicit DepthMapVisualizerShader();

    DepthMapVisualizerShader& bindTexture(Magnum::GL::Texture2D& texture) {
        texture.bind(TextureUnit);
        return *this;
    }

    private:
    enum: Magnum::Int { TextureUnit = 0 };
};

#endif
