#ifndef RAYMARCHEDCLOUDSSHADER_H
#define RAYMARCHEDCLOUDSSHADER_H

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Attribute.h>
#include <Magnum/Math/Color.h>
#include <Magnum/GL/Texture.h>

// Simple shader to render depth map to a quad for visualization
class RaymarchedCloudsShader : public Magnum::GL::AbstractShaderProgram {
    public:
    typedef Magnum::GL::Attribute<0, Magnum::Vector3> Position;
    typedef Magnum::GL::Attribute<1, Magnum::Vector2> TextureCoordinates;

    explicit RaymarchedCloudsShader();

    RaymarchedCloudsShader& setTime(float t);
    RaymarchedCloudsShader& setRes(float w, float h);
    RaymarchedCloudsShader& setDir(Magnum::Math::Vector3<float> dir);
    RaymarchedCloudsShader& setEye(Magnum::Math::Vector3<float> eye);
    RaymarchedCloudsShader& bindNoise() { _noiseTex->bind(TextureUnit); return *this; }

    void init();
    private:
    Magnum::GL::Texture2D *_noiseTex;
    const Magnum::Math::Vector2<int> _noiseTexSize{256, 256};
    enum: Magnum::Int { TextureUnit = 0 };
    Magnum::Int _timeUniform;
    Magnum::Int _resUniform;
    Magnum::Int _dirUniform;
    Magnum::Int _eyeUniform;
};

#endif
