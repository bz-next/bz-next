#include "DepthMapVisualizerShader.h"

#include <Magnum/GL/Version.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Shader.h>

#include <Corrade/Utility/Resource.h>

using namespace Magnum;

DepthMapVisualizerShader::DepthMapVisualizerShader() {
    MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL330);

    const Utility::Resource rs{"Shader-data"};

    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GL330, GL::Shader::Type::Fragment};

    vert.addSource(rs.getString("DepthMapVisualizerShader.vert"));
    frag.addSource(rs.getString("DepthMapVisualizerShader.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile() && frag.compile());

    attachShaders({vert, frag});

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    setUniform(uniformLocation("depthMap"), TextureUnit);
}
