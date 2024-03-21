#include "RaymarchedCloudsShader.h"

#include <Magnum/GL/Version.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Shader.h>

#include <Magnum/Trade/ImageData.h>

#include <Corrade/Utility/Resource.h>

#include "Magnum/GL/GL.h"
#include "Magnum/GL/Sampler.h"
#include "Magnum/Trade/Trade.h"
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/ImageView.h>
#include "common.h"

using namespace Magnum;

static void importShaderResources() {
    CORRADE_RESOURCE_INITIALIZE(SHADER_RESOURCES)
}

RaymarchedCloudsShader::RaymarchedCloudsShader() {
    Magnum::GL::Version shaderVersion;
    #ifdef TARGET_EMSCRIPTEN
    shaderVersion = GL::Version::GLES300;
    #else
    shaderVersion = GL::Version::GL330;
    #endif
    MAGNUM_ASSERT_GL_VERSION_SUPPORTED(shaderVersion);

    if(!Utility::Resource::hasGroup("Shader-data"))
        importShaderResources();

    const Utility::Resource rs{"Shader-data"};

    GL::Shader vert{shaderVersion, GL::Shader::Type::Vertex};
    GL::Shader frag{shaderVersion, GL::Shader::Type::Fragment};

    vert.addSource(rs.getString("compatibility.glsl"));
    vert.addSource(rs.getString("RaymarchedCloudsShader.vert"));
    frag.addSource(rs.getString("compatibility.glsl"));
    frag.addSource(rs.getString("RaymarchedCloudsShader.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile() && frag.compile());

    attachShaders({vert, frag});

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    setUniform(uniformLocation("iChannel0"), TextureUnit);
    _timeUniform = uniformLocation("u_time");
    _resUniform = uniformLocation("u_res");
    _lookAtUniform = uniformLocation("u_lookAt");
    _eyeUniform = uniformLocation("u_eye");
    _upUniform = uniformLocation("u_up");
    _sunDirUniform = uniformLocation("u_sunDir");
    _enableCloudsUniform = uniformLocation("u_enableClouds");
}

void RaymarchedCloudsShader::init() {
    Containers::Array<char> rgbaData{((unsigned)_noiseTexSize.x()*_noiseTexSize.y())*4};
    for (int i = 0; i < rgbaData.size(); i+=4) {
        rgbaData[i] = (unsigned int)((double)0xFF*bzfrand());
        rgbaData[i+2] = (unsigned int)((double)0xFF*bzfrand());
    }
    for (int i = 0; i < rgbaData.size(); i+=4) {
        int x = (i/4)%_noiseTexSize.x();
        int y = (i/4)/_noiseTexSize.x();
        int x2 = (x-37) &255;
        int y2 = (y-17) &255;

        rgbaData[4*(y*_noiseTexSize.x()+x)+1] = rgbaData[4*(y2*_noiseTexSize.x()+x2)];
        rgbaData[4*(y*_noiseTexSize.x()+x)+3] = rgbaData[4*(y2*_noiseTexSize.x()+x2)+2];
    }
    auto image = Trade::ImageData2D{PixelFormat::RGBA8Unorm, _noiseTexSize, std::move(rgbaData)};
    _noiseTex = new GL::Texture2D{};
    _noiseTex->setWrapping(GL::SamplerWrapping::Repeat)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear, GL::SamplerMipmap::Linear)
        .setMaxAnisotropy(GL::Sampler::maxMaxAnisotropy())
        .setStorage(8, GL::textureFormat(image.format()), image.size())
        .setSubImage(0, {}, image)
        .generateMipmap();
}

RaymarchedCloudsShader& RaymarchedCloudsShader::setTime(float t) {
    setUniform(_timeUniform, t);
    return *this;
}
RaymarchedCloudsShader& RaymarchedCloudsShader::setRes(float w, float h) {
    setUniform(_resUniform, Math::Vector2<float>{w, h});
    return *this;
}
RaymarchedCloudsShader& RaymarchedCloudsShader::setLookAt(Math::Vector3<float> lookAt) {
    setUniform(_lookAtUniform, lookAt);
    return *this;
}
RaymarchedCloudsShader& RaymarchedCloudsShader::setEye(Math::Vector3<float> eye) {
    setUniform(_eyeUniform, eye);
    return *this;
}
RaymarchedCloudsShader& RaymarchedCloudsShader::setUp(Math::Vector3<float> up) {
    setUniform(_upUniform, up);
    return *this;
}
RaymarchedCloudsShader& RaymarchedCloudsShader::setSunDir(Math::Vector3<float> sunDir) {
    setUniform(_sunDirUniform, sunDir);
    return *this;
}
RaymarchedCloudsShader& RaymarchedCloudsShader::setEnableClouds(bool enableClouds) {
    setUniform(_enableCloudsUniform, enableClouds);
    return *this;
}
