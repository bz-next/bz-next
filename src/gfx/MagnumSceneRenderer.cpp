#include "MagnumSceneRenderer.h"

#include "DrawableGroupManager.h"
#include "Magnum/GL/AbstractFramebuffer.h"
#include "Magnum/GL/Framebuffer.h"
#include "Magnum/GL/Sampler.h"
#include "Magnum/SceneGraph/Camera.h"

#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>

#include <string>

using namespace Magnum;

// Render scene from POV of camera using current drawmode and framebuffer
void MagnumSceneRenderer::renderScene(SceneGraph::Camera3D* camera) {
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
    if (auto* dg = DGRPMGR.getGroup("WorldDrawables"))
        camera->draw(*dg);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    if (auto* dg = DGRPMGR.getGroup("WorldTransDrawables"))
        camera->draw(*dg);
}

void MagnumSceneRenderer::renderLightDepthMap(SceneGraph::Camera3D* lightCamera) {
    // Set up camera
    (*lightCamera)
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::orthographicProjection({(float)_depthMapSize[0], (float)_depthMapSize[1]}, 1.0f, 1000.0f))
        .setViewport(_depthMapSize);
    // Set up renderbuffer / framebuffer
    GL::Texture2D depthTex;
    depthTex
        .setWrapping(GL::SamplerWrapping::Repeat)
        .setMagnificationFilter(GL::SamplerFilter::Nearest)
        .setMinificationFilter(GL::SamplerFilter::Nearest)
        .setStorage(1, GL::TextureFormat::DepthComponent16, _depthMapSize);
    GL::Framebuffer depthFB{{{}, _depthMapSize}};
    depthFB.attachTexture(GL::Framebuffer::BufferAttachment::Depth, depthTex, 0);

    depthFB.clear(GL::FramebufferClear::Depth).bind();

    GL::defaultFramebuffer.bind();
}
