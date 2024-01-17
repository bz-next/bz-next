#ifndef TEXTUREDQUADSHADER_H
#define TEXTUREDQUADSHADER_H

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Attribute.h>
#include <Magnum/GL/Texture.h>

class TexturedQuadShader: public Magnum::GL::AbstractShaderProgram {
    public:
        typedef Magnum::GL::Attribute<0, Magnum::Vector2> Position;
        typedef Magnum::GL::Attribute<1, Magnum::Vector2> TextureCoordinates;

        explicit TexturedQuadShader();

        TexturedQuadShader& setColor(const Magnum::Color3& color) {
            setUniform(_colorUniform, color);
            return *this;
        }

        TexturedQuadShader& bindTexture(GL::Texture2D& texture) {
            texture.bind(TextureUnit);
            return *this;
        }
    private:
        enum: Magnum::Int { TextureUnit = 0 };

        Magnum::Int _colorUniform;
};

#endif