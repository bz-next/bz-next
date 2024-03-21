#ifndef DEPTHREINTERPRETSHADER_H
#define DEPTHREINTERPRETSHADER_H

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Attribute.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/GL/Texture.h>

class DepthReinterpretShader: public Magnum::GL::AbstractShaderProgram {
    public:
        explicit DepthReinterpretShader(Magnum::NoCreateT): Magnum::GL::AbstractShaderProgram{Magnum::NoCreate} {}
        explicit DepthReinterpretShader();

        DepthReinterpretShader& bindDepthTexture(Magnum::GL::Texture2D& texture) {
            texture.bind(TextureUnit);
            return *this;
        }
        enum: Magnum::Int { TextureUnit = 0 };
};

#endif
