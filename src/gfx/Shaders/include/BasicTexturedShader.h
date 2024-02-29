#ifndef BASICTEXTUREDSHADER_H
#define BASICTEXTUREDSHADER_H

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Attribute.h>
#include <Magnum/Math/Color.h>
#include <Magnum/GL/Texture.h>

// This is an example of a basic textured shader,
// comprised of a vertex and fragment shader.
// You can use it as a template to develop new shaders.
// References shader programs in resources/BasicTexturedShader.vert and
// BasicTexturedShader.frag
// These are compiled in using Magnum/Corrade and accessible as Resources
class BasicTexturedShader : public Magnum::GL::AbstractShaderProgram {
    public:
    typedef Magnum::GL::Attribute<0, Magnum::Vector3> Position;
    typedef Magnum::GL::Attribute<1, Magnum::Vector2> TextureCoordinates;

    explicit BasicTexturedShader();

    BasicTexturedShader& setColor(const Magnum::Color3& color) {
        setUniform(_colorUniform, color);
        return *this;
    }

    BasicTexturedShader& bindTexture(Magnum::GL::Texture2D& texture) {
        texture.bind(TextureUnit);
        return *this;
    }

    private:
    enum: Magnum::Int { TextureUnit = 0 };
    Magnum::Int _colorUniform;
};

#endif
