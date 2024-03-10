#include "MagnumSceneRenderer.h"

#include "DrawableGroupManager.h"
#include "Magnum/GL/AbstractFramebuffer.h"
#include "Magnum/GL/Framebuffer.h"
#include "Magnum/GL/GL.h"
#include "Magnum/GL/Sampler.h"
#include "Magnum/SceneGraph/Camera.h"

#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>

#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/Trade/MeshData.h>

#include <Magnum/SceneGraph/Camera.h>

#include <string>

#include "DrawModeManager.h"
#include "MagnumTextureManager.h"
#include "SceneObjectManager.h"

using namespace Magnum;

Object3D* MagnumSceneRenderer::_lightObj = NULL;

MagnumSceneRenderer::MagnumSceneRenderer() {}

void MagnumSceneRenderer::init() {
    // Add depth map texture
    {
        GL::Texture2D *tex = new GL::Texture2D{};
        (*tex)
            .setWrapping(GL::SamplerWrapping::Repeat)
            .setMagnificationFilter(GL::SamplerFilter::Nearest)
            .setMinificationFilter(GL::SamplerFilter::Nearest)
            .setStorage(1, GL::TextureFormat::DepthComponent16, _depthMapSize);
        addPipelineTex("DepthMapTex", {tex, (unsigned)_depthMapSize[0], (unsigned)_depthMapSize[1], false});
    }
    // Add depth map preview texture
    // This is just the 16-bit depth map rendered to a regular rgba texture
    // for previewing
    {
        GL::Texture2D *tex = new GL::Texture2D{};
        (*tex)
            .setWrapping(GL::SamplerWrapping::Repeat)
            .setMagnificationFilter(GL::SamplerFilter::Nearest)
            .setMinificationFilter(GL::SamplerFilter::Nearest)
            .setStorage(1, GL::TextureFormat::RGBA8, _depthMapSize);
        addPipelineTex("DepthMapPreviewTex", {tex, (unsigned)_depthMapSize[0], (unsigned)_depthMapSize[1], false});
    }

    // Create a generic quad mesh for previewing
    const Vector3 vertices[]{
        {{ 1.0f, -1.0f, 0.0f}}, /* Bottom right */
        {{ 1.0f,  1.0f, 0.0f}}, /* Top right */
        {{-1.0f, -1.0f, 0.0f}}, /* Bottom left */
        {{-1.0f,  1.0f, 0.0f}}  /* Top left */
    };
    const Vector2 texcoords[]{
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 0.0f},
        {0.0f, 1.0f}
    };
    const UnsignedInt indices[]{        /* 3--1 1 */
        0, 1, 2,            /* | / /| */
        2, 1, 3             /* |/ / | */
    };

    struct VertexData {
        Vector3 position;
        Vector2 texcoord;
    };

    Containers::ArrayView<const Vector3> posview{vertices};
    Containers::ArrayView<const Vector2> texview{texcoords};

    // Pack mesh data
    Containers::Array<char> data{posview.size()*sizeof(VertexData)};
    data = MeshTools::interleave(posview, texview);
    Containers::StridedArrayView1D<const VertexData> dataview = Containers::arrayCast<const VertexData>(data);

    Trade::MeshData mdata{MeshPrimitive::Triangles, Trade::DataFlags{}, indices, Trade::MeshIndexData{indices}, std::move(data), {
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, dataview.slice(&VertexData::position)},
        Trade::MeshAttributeData{Trade::MeshAttribute::TextureCoordinates, dataview.slice(&VertexData::texcoord)},
    }, static_cast<UnsignedInt>(dataview.size())};

    _quadMesh = MeshTools::compile(mdata);

    Object3D* lightobj = SOMGR.getObj("Sun");
    lightobj->resetTransformation();
    lightobj->transform(Matrix4::lookAt({500.0f, 500.0f, 500.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}));

    _lightCamera = new SceneGraph::Camera3D{*lightobj};
    _bzmatMode.setLightObj(lightobj);
}

TextureData MagnumSceneRenderer::getPipelineTex(const std::string& name) {
    auto it = _pipelineTexMap.find(name);
    if (it != _pipelineTexMap.end()) {
        return it->second;
    }
    return {NULL, 0, 0, false};
}

void MagnumSceneRenderer::addPipelineTex(const std::string& name, TextureData data) {
    _pipelineTexMap.insert(std::make_pair(name, data));
}

// Render scene from POV of camera using current drawmode and framebuffer
void MagnumSceneRenderer::renderScene(SceneGraph::Camera3D* camera) {
    DRAWMODEMGR.setDrawMode(&_bzmatMode);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
    if (auto* dg = DGRPMGR.getGroup("WorldDrawables"))
        camera->draw(*dg);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    //if (auto* dg = DGRPMGR.getGroup("TankDrawables"))
    //    _camera->draw(*dg);
    if (auto* dg = DGRPMGR.getGroup("WorldTransDrawables"))
        camera->draw(*dg);
   
}

void MagnumSceneRenderer::renderLightDepthMap() {
    TextureData depthTexData = getPipelineTex("DepthMapTex");
    // Much of this should only be done when the sun moves.
    // Set up camera
    (*_lightCamera)
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::orthographicProjection({2048.0f, 2048.0f}, 10.0f, 2000.0f))
        .setViewport({(int)depthTexData.width, (int)depthTexData.height});
    // Set up renderbuffer / framebuffer
    GL::Framebuffer depthFB{{{}, {(int)depthTexData.width, (int)depthTexData.height}}};
    depthFB.attachTexture(GL::Framebuffer::BufferAttachment::Depth, *depthTexData.texture, 0);

    DRAWMODEMGR.setDrawMode(&_depthMode);

    depthFB.clear(GL::FramebufferClear::Depth).bind();

    // Now render to texture
    if (auto* dg = DGRPMGR.getGroup("WorldDrawables"))
        _lightCamera->draw(*dg);

    GL::defaultFramebuffer.bind();
}

// Render the 16-bit depth buffer to a regular rgba texture for presentation
// Not really necessary, but a good demo on how to do something like this.
void MagnumSceneRenderer::renderLightDepthMapPreview() {
    TextureData depthTexData = getPipelineTex("DepthMapTex");
    TextureData depthPreviewTexData = getPipelineTex("DepthMapPreviewTex");

    GL::Renderbuffer depth;
    depth.setStorage(GL::RenderbufferFormat::DepthComponent16, {(int)depthPreviewTexData.width, (int)depthPreviewTexData.height});
    GL::Framebuffer framebuffer{ {{}, {(int)depthPreviewTexData.width, (int)depthPreviewTexData.height}} };
    framebuffer.attachTexture(GL::Framebuffer::ColorAttachment{ 0 }, *depthPreviewTexData.texture, 0);

    framebuffer.attachRenderbuffer(
    GL::Framebuffer::BufferAttachment::Depth, depth);

    framebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth).bind();

    _depthVisShader
        .bindTexture(*depthTexData.texture)
        .draw(_quadMesh);

    GL::defaultFramebuffer.bind();
}
