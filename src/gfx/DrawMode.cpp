#include "DrawMode.h"

#include <Magnum/SceneGraph/Camera.hpp>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/TextureFormat.h>
#include "BasicTexturedShader.h"
#include "Magnum/GL/Renderer.h"
#include "Magnum/Shaders/PhongGL.h"
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/Math/Color.h>

#include "MagnumBZMaterial.h"
#include "MagnumTextureManager.h"

#include "DynamicColor.h"
#include "TextureMatrix.h"

#include "MagnumSceneRenderer.h"
#include "SceneObjectManager.h"

using namespace Magnum;

#define MAGNUMROWCOL(r, c) (r+c*3)
#define INTROWCOL(r, c) (r+c*4)

BZMaterialDrawMode::BZMaterialDrawMode() {
    _shader = new Magnum::Shaders::PhongGL{Magnum::Shaders::PhongGL::Configuration{}
        .setFlags(
            Magnum::Shaders::PhongGL::Flag::DiffuseTexture |
            Magnum::Shaders::PhongGL::Flag::AmbientTexture |
            Magnum::Shaders::PhongGL::Flag::AlphaMask |
            Magnum::Shaders::PhongGL::Flag::TextureTransformation)};
    _shaderUntex = new Magnum::Shaders::PhongGL{Magnum::Shaders::PhongGL::Configuration{}};
}

void BZMaterialDrawMode::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera, const MagnumBZMaterial* mat, GL::Mesh& mesh)  {
    MagnumTextureManager &tm = MagnumTextureManager::instance();

    auto toMagnumColor = [](const float *cp) {
        return Color3{cp[0], cp[1], cp[2]};
    };

    // NULL material just skips render
    if (mat) {
        Math::Vector3<float> abstranslation = {0.0f, 0.0f, -10.0f};   // Default light position behind camera if sun does not exist
        if (_lightObj)
            abstranslation = camera.cameraMatrix().transformPoint(_lightObj->absoluteTransformationMatrix().translation());

        Color3 dyncol;
        if (mat->getDynamicColor() != -1) {
            auto * dc = DYNCOLORMGR.getColor(mat->getDynamicColor());
            if (dc) {
                dyncol = toMagnumColor(dc->getColor());
            }
        }

        if (mat->getNoCulling())
            GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
        else
            GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

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
            
            (*_shader).bindDiffuseTexture(*t.texture)
                .bindAmbientTexture(*t.texture)
                .setDiffuseColor(Color4{toMagnumColor(mat->getDiffuse()), 0.0f})
                .setAmbientColor(mat->getNoLighting() ? Color4{1.0f*toMagnumColor(mat->getDiffuse()) + toMagnumColor(mat->getEmission()) + dyncol, mat->getDiffuse()[3]} : Color4{0.2*toMagnumColor(mat->getAmbient()) + toMagnumColor(mat->getEmission()) + dyncol, mat->getDiffuse()[3]})
                .setSpecularColor(Color4{toMagnumColor(mat->getSpecular()), 0.0f})
                .setShininess(mat->getShininess())
                .setAlphaMask(alphathresh)
                .setNormalMatrix(transformationMatrix.normalMatrix())
                .setTransformationMatrix(transformationMatrix)
                .setProjectionMatrix(camera.projectionMatrix())
                .setLightPositions({
                    {abstranslation, 0.0f}
                })
                .setTextureMatrix(texmat)
                .draw(mesh);
        } else {
            (*_shaderUntex)
                .setDiffuseColor(Color4{toMagnumColor(mat->getDiffuse()), 0.0f})
                .setAmbientColor(mat->getNoLighting() ? Color4{1.0f*toMagnumColor(mat->getDiffuse()) + toMagnumColor(mat->getEmission()) + dyncol, mat->getDiffuse()[3]} : Color4{0.2*toMagnumColor(mat->getAmbient()) + toMagnumColor(mat->getEmission()) + dyncol, mat->getDiffuse()[3]})
                .setSpecularColor(Color4{toMagnumColor(mat->getSpecular()), 0.0f})
                .setShininess(mat->getShininess())
                .setNormalMatrix(transformationMatrix.normalMatrix())
                .setTransformationMatrix(transformationMatrix)
                .setProjectionMatrix(camera.projectionMatrix())
                .setLightPositions({
                    {abstranslation, 0.0f}
                })
                .draw(mesh);
        }
    }
}

BasicTexturedShaderDrawMode::BasicTexturedShaderDrawMode() {
    _shader = new BasicTexturedShader();
}

void BasicTexturedShaderDrawMode::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera, const MagnumBZMaterial* mat, GL::Mesh& mesh)  {
    MagnumTextureManager &tm = MagnumTextureManager::instance();

    auto toMagnumColor = [](const float *cp) {
        return Color3{cp[0], cp[1], cp[2]};
    };

    // NULL material just skips render
    if (mat) {

        Color3 dyncol;
        if (mat->getDynamicColor() != -1) {
            auto * dc = DYNCOLORMGR.getColor(mat->getDynamicColor());
            if (dc) {
                dyncol = toMagnumColor(dc->getColor());
            }
        }

        if (mat->getNoCulling())
            GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
        else
            GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

        TextureData t = tm.getTexture(mat->getTexture(0).c_str());
        if (t.texture) {
            (*_shader).bindTexture(*t.texture)
                .setColor(Color3{toMagnumColor(mat->getDiffuse())})
                .draw(mesh);
        }
    }
}

DepthMapDrawMode::DepthMapDrawMode() {
    _shader = new DepthMapShader();
}

void DepthMapDrawMode::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera, const MagnumBZMaterial* mat, GL::Mesh& mesh)  {
    // NULL material just skips render
    // Perhaps exclude transparent materials here to simplify shadow rendering...
    if (mat) {

        if (mat->getNoCulling())
            GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
        else
            GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

        (*_shader)
            .setTransformationMatrix(transformationMatrix)
            .setProjectionMatrix(camera.projectionMatrix())
            .draw(mesh);
    }
}
