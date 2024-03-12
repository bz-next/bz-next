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
    MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL330);

    if(!Utility::Resource::hasGroup("Shader-data"))
        importShaderResources();

    const Utility::Resource rs{"Shader-data"};

    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GL330, GL::Shader::Type::Fragment};

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
}

void RaymarchedCloudsShader::init() {
    Containers::Array<char> rgbaData{((unsigned)_noiseTexSize.x()*_noiseTexSize.y())*4};
    for (int i = 0; i < rgbaData.size(); ++i) {
        rgbaData[i] = (unsigned char)((double)0xFF*bzfrand());
    }
    auto image = Trade::ImageData2D{PixelFormat::RGBA8Unorm, _noiseTexSize, std::move(rgbaData)};
    _noiseTex = new GL::Texture2D{};
    _noiseTex->setWrapping(GL::SamplerWrapping::Repeat)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear, GL::SamplerMipmap::Linear)
        .setMaxAnisotropy(GL::Sampler::maxMaxAnisotropy())
        .setStorage(4, GL::textureFormat(image.format()), image.size())
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