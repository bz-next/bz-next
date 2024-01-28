#include "BZMaterialViewer.h"
#include "Magnum/GL/AbstractFramebuffer.h"
#include "Magnum/GL/DefaultFramebuffer.h"
#include "Magnum/GL/Framebuffer.h"
#include "Magnum/GL/GL.h"
#include "MagnumBZMaterial.h"
#include <imgui.h>

#include <Magnum/GL/Texture.h>
#include <Magnum/ImGuiIntegration/Widgets.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Vector.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>

#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/ImageView.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/GL/Context.h>

using namespace Magnum;
using namespace Magnum::Math::Literals;

MatViewerShader::MatViewerShader() {
    MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL330);

    const Utility::Resource rs{"matviewer-data"};

    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GL330, GL::Shader::Type::Fragment};

    vert.addSource(rs.getString("MatViewer.vert"));
    frag.addSource(rs.getString("MatViewer.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile() && frag.compile());

    attachShaders({vert, frag});

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    _colorUniform = uniformLocation("color");
    setUniform(uniformLocation("textureData"), TextureUnit);
}

BZMaterialViewer::BZMaterialViewer() {
    
    renderTex
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::TextureFormat::RGBA8, bufsize);

    itemCurrent = 0;

    struct QuadVertex {
        Vector2 position;
        Vector2 textureCoordinates;
    };
    const QuadVertex vertices[]{
        {{ 0.5f, -0.5f}, {1.0f, 0.0f}}, /* Bottom right */
        {{ 0.5f,  0.5f}, {1.0f, 1.0f}}, /* Top right */
        {{-0.5f, -0.5f}, {0.0f, 0.0f}}, /* Bottom left */
        {{-0.5f,  0.5f}, {0.0f, 1.0f}}  /* Top left */
    };
    const UnsignedInt indices[]{        /* 3--1 1 */
        0, 1, 2,                        /* | / /| */
        2, 1, 3                         /* |/ / | */
    };

    mesh.setCount(Containers::arraySize(indices))
        .addVertexBuffer(GL::Buffer{vertices}, 0,
            MatViewerShader::Position{},
            MatViewerShader::TextureCoordinates{})
        .setIndexBuffer(GL::Buffer{indices}, 0,
            GL::MeshIndexType::UnsignedInt);
    
    struct TriangleVertex {
        Vector2 position;
        Color3 color;
    };
    const TriangleVertex vertices2[]{
        {{-0.5f, -0.5f}, 0xff0000_rgbf},    /* Left vertex, red color */
        {{ 0.5f, -0.5f}, 0x00ff00_rgbf},    /* Right vertex, green color */
        {{ 0.0f,  0.5f}, 0x0000ff_rgbf}     /* Top vertex, blue color */
    };
    testtri.setCount(Containers::arraySize(vertices2))
         .addVertexBuffer(GL::Buffer{vertices2}, 0,
            Shaders::VertexColorGL2D::Position{},
            Shaders::VertexColorGL2D::Color3{});
}

void BZMaterialViewer::draw(const char *title, bool *p_open) {
    std::vector<std::string> names = MAGNUMMATERIALMGR.getMaterialNames();
    std::string names_cc;
    for (const auto& e: names) {
        names_cc += e + std::string("\0", 1);
    }
    renderPreview();
    ImGui::Begin(title, p_open, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Combo("Material Name", &itemCurrent, names_cc.c_str(), names_cc.size());
        ImGui::Separator();
        ImGuiIntegration::image(renderTex, {(float)bufsize[0], (float)bufsize[1]});
    ImGui::End();
}

void BZMaterialViewer::renderPreview() {
    GL::Renderbuffer depthStencil;
    depthStencil.setStorage(GL::RenderbufferFormat::Depth24Stencil8, bufsize);
    GL::Framebuffer framebuffer{ {{}, bufsize} };
    framebuffer.attachTexture(GL::Framebuffer::ColorAttachment{ 0 }, renderTex, 0);

    framebuffer.attachRenderbuffer(
    GL::Framebuffer::BufferAttachment::DepthStencil, depthStencil);

    framebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth).bind();

    MagnumTextureManager &tm = MagnumTextureManager::instance();

    std::vector<std::string> names = MAGNUMMATERIALMGR.getMaterialNames();

    if (itemCurrent < names.size()) {
        const MagnumBZMaterial *mat = MAGNUMMATERIALMGR.findMaterial(names[itemCurrent].c_str());
        if (mat) {
            GL::Texture2D *t = tm.getTexture(mat->getTexture(0).c_str());
            if (t) {
                shader.setColor(0xffb2b2_rgbf).bindTexture(*t).draw(mesh);
                
            }
        }
    }
    GL::defaultFramebuffer.bind();
}