#include "BZMaterialViewer.h"
#include "Corrade/Containers/ArrayView.h"
#include "DynamicColor.h"
#include "Magnum/GL/AbstractFramebuffer.h"
#include "Magnum/GL/DefaultFramebuffer.h"
#include "Magnum/GL/Framebuffer.h"
#include "Magnum/GL/GL.h"
#include "Magnum/Mesh.h"
#include "Magnum/Shaders/PhongGL.h"
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
#include <Magnum/Shaders/Generic.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Duplicate.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/Trade/MeshData.h>

#include "TextureMatrix.h"

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

BZMaterialViewer::BZMaterialViewer() :
    phong {
        Shaders::PhongGL::Configuration{}
            .setFlags(Shaders::PhongGL::Flag::DiffuseTexture | Shaders::PhongGL::Flag::AmbientTexture | Shaders::PhongGL::Flag::UniformBuffers | Shaders::PhongGL::Flag::AlphaMask | Shaders::PhongGL::Flag::TextureTransformation)
            .setMaterialCount(1)
            .setLightCount(1)
            .setDrawCount(1)
    },
    phongUntex {
        Shaders::PhongGL::Configuration{}
            .setFlags(Shaders::PhongGL::Flag::UniformBuffers | Shaders::PhongGL::Flag::AlphaMask)
            .setMaterialCount(1)
            .setLightCount(1)
            .setDrawCount(1)
    } {
    
    renderTex
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::TextureFormat::RGBA8, bufsize);

    itemCurrent = 0;

    const Vector3 vertices[]{
        {{ 0.5f, -0.5f, 0.0f}}, /* Bottom right */
        {{ 0.5f,  0.5f, 0.0f}}, /* Top right */
        {{-0.5f, -0.5f, 0.0f}}, /* Bottom left */
        {{-0.5f,  0.5f, 0.0f}}  /* Top left */
    };
    const Vector2 texcoords[]{
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 0.0f},
        {0.0f, 1.0f}
    };
    const Vector3 normals[]{
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f}
    };
    const UnsignedInt indices[]{        /* 3--1 1 */
        0, 1, 2,                        /* | / /| */
        2, 1, 3                         /* |/ / | */
    };

    struct VertexData {
        Vector3 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    //Containers::Array<Vector3> positions = MeshTools::duplicate<typeof indices[0], Vector3>(indices, vertices);
    Containers::ArrayView<const Vector3> posview{vertices};
    Containers::ArrayView<const Vector2> texview{texcoords};
    Containers::ArrayView<const Vector3> normview{normals};

    // Pack mesh data
    Containers::Array<char> data{posview.size()*sizeof(VertexData)};
    data = MeshTools::interleave(posview, texview, normview);
    Containers::StridedArrayView1D<const VertexData> dataview = Containers::arrayCast<const VertexData>(data);

    Trade::MeshData mdata{MeshPrimitive::Triangles, Trade::DataFlags{}, indices, Trade::MeshIndexData{indices}, std::move(data), {
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, dataview.slice(&VertexData::position)},
        Trade::MeshAttributeData{Trade::MeshAttribute::TextureCoordinates, dataview.slice(&VertexData::texcoord)},
        Trade::MeshAttributeData{Trade::MeshAttribute::Normal, dataview.slice(&VertexData::normal)}
    }, static_cast<UnsignedInt>(dataview.size())};

    mesh = MeshTools::compile(mdata);
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

Color3 toMagnumColor(const float *cp) {
    return Color3{cp[0], cp[1], cp[2]};
}

#define MAGNUMROWCOL(r, c) (r+c*3)
#define INTROWCOL(r, c) (r+c*4)

void BZMaterialViewer::renderPreview() {
    static float tt = 0.0;
    GL::Renderbuffer depthStencil;
    depthStencil.setStorage(GL::RenderbufferFormat::Depth24Stencil8, bufsize);
    GL::Framebuffer framebuffer{ {{}, bufsize} };
    framebuffer.attachTexture(GL::Framebuffer::ColorAttachment{ 0 }, renderTex, 0);

    framebuffer.attachRenderbuffer(
    GL::Framebuffer::BufferAttachment::DepthStencil, depthStencil);

    framebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth).bind();

    MagnumTextureManager &tm = MagnumTextureManager::instance();

    std::vector<std::string> names = MAGNUMMATERIALMGR.getMaterialNames();

    GL::Buffer projectionUniform, lightUniform, materialUniform, transformationUniform, drawUniform, textureTransformationUniform;
    Math::Matrix4<float> normalMat;

    Matrix4 transformationMatrix = Matrix4::translation(Vector3::zAxis(-2.0f));
    Matrix4 projectionMatrix = Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.001f, 100.0f);

    projectionUniform.setData({
        Shaders::ProjectionUniform3D{}
            .setProjectionMatrix(projectionMatrix)
    });
    lightUniform.setData({
        Shaders::PhongLightUniform{}
            .setPosition({0.0, 0.0, 1.0, 0.0})
            .setColor({1.0, 1.0, 1.0})
    });
    transformationUniform.setData({
        Shaders::TransformationUniform3D{}
            .setTransformationMatrix(transformationMatrix)
    });
    drawUniform.setData({
        Shaders::PhongDrawUniform{}
            .setNormalMatrix(transformationMatrix.normalMatrix())
            .setMaterialId(0)
    });


    if (itemCurrent < names.size()) {
        const MagnumBZMaterial *mat = MAGNUMMATERIALMGR.findMaterial(names[itemCurrent].c_str());
        if (mat) {
            Color3 dyncol;
            if (mat->getDynamicColor() != -1) {
                auto * dc = DYNCOLORMGR.getColor(mat->getDynamicColor());
                if (dc) {
                    dyncol = toMagnumColor(dc->getColor());
                }
            }
            materialUniform.setData({
                Shaders::PhongMaterialUniform{}
                    .setDiffuseColor(Color4{toMagnumColor(mat->getDiffuse()), 0.0f})
                    .setAmbientColor(toMagnumColor(mat->getAmbient()) + toMagnumColor(mat->getEmission()) + dyncol)
                    .setSpecularColor(Color4{toMagnumColor(mat->getSpecular()), 0.0f})
                    .setShininess(mat->getShininess())
                    .setAlphaMask(mat->getAlphaThreshold())
            });

            GL::Texture2D *t = tm.getTexture(mat->getTexture(0).c_str());
            if (t) {
                    textureTransformationUniform.setData({
                        Shaders::TextureTransformationUniform{}
                            .setTextureMatrix(Matrix3{})
                    });
                if (mat->getTextureMatrix(0) != -1) {
                    const TextureMatrix *texmat_internal = TEXMATRIXMGR.getMatrix(mat->getTextureMatrix(0));
                    Matrix3 texmat;
                    
                    auto &tmd = texmat.data();
                    const float *tmid = texmat_internal->getMatrix();

                    // BZFlag TextureMatrix packs the data weirdly
                    tmd[MAGNUMROWCOL(0, 0)] = tmid[INTROWCOL(0, 0)];
                    tmd[MAGNUMROWCOL(0, 1)] = tmid[INTROWCOL(0, 1)];
                    tmd[MAGNUMROWCOL(1, 0)] = tmid[INTROWCOL(1, 0)];
                    tmd[MAGNUMROWCOL(1, 1)] = tmid[INTROWCOL(1, 1)];
                    tmd[MAGNUMROWCOL(0, 2)] = tmid[INTROWCOL(0, 3)];
                    tmd[MAGNUMROWCOL(1, 2)] = tmid[INTROWCOL(1, 3)];
                    
                    textureTransformationUniform.setData({
                        Shaders::TextureTransformationUniform{}
                            .setTextureMatrix(texmat)
                    });
                }
                phong.bindDiffuseTexture(*t)
                    .bindAmbientTexture(*t)
                    .bindLightBuffer(lightUniform)
                    .bindProjectionBuffer(projectionUniform)
                    .bindMaterialBuffer(materialUniform)
                    .bindTransformationBuffer(transformationUniform)
                    .bindDrawBuffer(drawUniform)
                    .bindTextureTransformationBuffer(textureTransformationUniform)
                    .draw(mesh);
            } else {
                phongUntex
                    .bindLightBuffer(lightUniform)
                    .bindProjectionBuffer(projectionUniform)
                    .bindMaterialBuffer(materialUniform)
                    .bindTransformationBuffer(transformationUniform)
                    .bindDrawBuffer(drawUniform)
                    .draw(mesh);
            }
        }
    }
    GL::defaultFramebuffer.bind();
}