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

BZMaterialViewer::BZMaterialViewer() :
    phong {
        Shaders::PhongGL::Configuration{}
            .setFlags(
                Shaders::PhongGL::Flag::DiffuseTexture |
                Shaders::PhongGL::Flag::AmbientTexture |
                Shaders::PhongGL::Flag::AlphaMask |
                Shaders::PhongGL::Flag::TextureTransformation)},
    phongUntex {
        Shaders::PhongGL::Configuration{}}
    
    {
    
    renderTex
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear)
#ifdef MAGNUM_TARGET_GLES2
        .setStorage(1, GL::TextureFormat::RGBA8, bufsize);
#else
        .setStorage(1, GL::TextureFormat::RGBA8, bufsize);
#endif

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

// TODO: Make this use the BZ Material Drawable. Requires the widget to have its own scenegraph
// But this would make this more useful, since the preview would always be in sync with the actual renderer code
void BZMaterialViewer::renderPreview() {
    static float tt = 0.0;
    GL::Renderbuffer depth;
    depth.setStorage(GL::RenderbufferFormat::DepthComponent16, bufsize);
    GL::Framebuffer framebuffer{ {{}, bufsize} };
    framebuffer.attachTexture(GL::Framebuffer::ColorAttachment{ 0 }, renderTex, 0);

    framebuffer.attachRenderbuffer(
    GL::Framebuffer::BufferAttachment::Depth, depth);

    framebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth).bind();

    MagnumTextureManager &tm = MagnumTextureManager::instance();

    std::vector<std::string> names = MAGNUMMATERIALMGR.getMaterialNames();

    Math::Matrix4<float> normalMat;

    Matrix4 transformationMatrix = Matrix4::translation(Vector3::zAxis(-2.0f));
    Matrix4 projectionMatrix = Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.001f, 100.0f);


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
            

            TextureData t = tm.getTexture(mat->getTexture(0).c_str());
            if (t.texture) {
                    float alphathresh = mat->getAlphaThreshold();
            // The map "duck dodgers" set alphathresh=1 for many materials
            // and the old client still rendered them. Max out alphathresh at
            // 0.999, since some map makers might just max out the value
            // to get transparency working.
            if (alphathresh > 0.999f) alphathresh = 0.999f;

            Matrix3 texmat;

            if (mat->getTextureMatrix(0) != -1) {
                const TextureMatrix *texmat_internal = TEXMATRIXMGR.getMatrix(mat->getTextureMatrix(0));
                
                
                auto &tmd = texmat.data();
                const float *tmid = texmat_internal->getMatrix();

                // BZFlag TextureMatrix packs the data weirdly
                tmd[MAGNUMROWCOL(0, 0)] = tmid[INTROWCOL(0, 0)];
                tmd[MAGNUMROWCOL(0, 1)] = tmid[INTROWCOL(0, 1)];
                tmd[MAGNUMROWCOL(1, 0)] = tmid[INTROWCOL(1, 0)];
                tmd[MAGNUMROWCOL(1, 1)] = tmid[INTROWCOL(1, 1)];
                tmd[MAGNUMROWCOL(0, 2)] = tmid[INTROWCOL(0, 3)];
                tmd[MAGNUMROWCOL(1, 2)] = tmid[INTROWCOL(1, 3)];
                
            }
            phong.bindDiffuseTexture(*t.texture)
                .bindAmbientTexture(*t.texture)
                .setDiffuseColor(Color4{toMagnumColor(mat->getDiffuse()), 0.0f})
                .setAmbientColor(mat->getNoLighting() ? Color4{1.0f*toMagnumColor(mat->getDiffuse()) + toMagnumColor(mat->getEmission()) + dyncol, mat->getDiffuse()[3]} : Color4{0.2*toMagnumColor(mat->getAmbient()) + toMagnumColor(mat->getEmission()) + dyncol, mat->getDiffuse()[3]})
                .setSpecularColor(Color4{toMagnumColor(mat->getSpecular()), 0.0f})
                .setShininess(mat->getShininess())
                .setAlphaMask(alphathresh)
                .setNormalMatrix(transformationMatrix.normalMatrix())
                .setTransformationMatrix(transformationMatrix)
                .setProjectionMatrix(projectionMatrix)
                .setLightPositions({{0.0, 0.0, 1.0, 0.0}})
                .setTextureMatrix(texmat)
                .draw(mesh);
            } else {
                phongUntex
                    .setDiffuseColor(Color4{toMagnumColor(mat->getDiffuse()), 0.0f})
                    .setAmbientColor(mat->getNoLighting() ? Color4{1.0f*toMagnumColor(mat->getDiffuse()) + toMagnumColor(mat->getEmission()) + dyncol, mat->getDiffuse()[3]} : Color4{0.2*toMagnumColor(mat->getAmbient()) + toMagnumColor(mat->getEmission()) + dyncol, mat->getDiffuse()[3]})
                    .setSpecularColor(Color4{toMagnumColor(mat->getSpecular()), 0.0f})
                    .setShininess(mat->getShininess())
                    .setNormalMatrix(transformationMatrix.normalMatrix())
                    .setTransformationMatrix(transformationMatrix)
                    .setProjectionMatrix(projectionMatrix)
                    .setLightPositions({{0.0, 0.0, 1.0, 0.0}})
                    .draw(mesh);
            }
        }
    }
    GL::defaultFramebuffer.bind();
}