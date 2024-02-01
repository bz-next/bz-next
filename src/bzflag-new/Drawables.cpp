#include "Drawables.h"

#include <Magnum/SceneGraph/Camera.hpp>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/TextureFormat.h>
#include "Magnum/GL/Renderer.h"
#include "Magnum/Shaders/PhongGL.h"
#include "Magnum/Shaders/Phong.h"
#include "Magnum/Shaders/Generic.h"
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/DefaultFramebuffer.h>

#include "MagnumBZMaterial.h"
#include "MagnumTextureManager.h"

#include "DynamicColor.h"
#include "TextureMatrix.h"

using namespace Magnum;

void InstancedColoredDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setLightPositions({{camera.cameraMatrix().transformPoint({0.0f, 0.0f, 1000.0f}), 0.0f}})
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .draw(_mesh);
}

void ColoredDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setDiffuseColor(_color)
        .setLightPositions({{camera.cameraMatrix().transformPoint({0.0f, 0.0f, 1000.0f}), 0.0f}})
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .draw(_mesh);
}

void TexturedDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setLightPositions({{camera.cameraMatrix().transformPoint({0.0f, 0.0f, 1000.0f}), 0.0f}})
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .bindDiffuseTexture(_texture)
        .draw(_mesh);
}

#define MAGNUMROWCOL(r, c) (r+c*3)
#define INTROWCOL(r, c) (r+c*4)

void BZMaterialDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    MagnumTextureManager &tm = MagnumTextureManager::instance();
    GL::Buffer projectionUniform, lightUniform, materialUniform, transformationUniform, drawUniform, textureTransformationUniform;

    const MagnumBZMaterial *mat = MAGNUMMATERIALMGR.findMaterial(_matName);
    
    projectionUniform.setData({
        Shaders::ProjectionUniform3D{}
            .setProjectionMatrix(camera.projectionMatrix())
    });
    lightUniform.setData({
        Shaders::PhongLightUniform{}
            .setPosition({camera.cameraMatrix().transformPoint({0.0f, 0.0f, 1000.0f}), 0.0f})
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

    auto toMagnumColor = [](const float *cp) {
        return Color3{cp[0], cp[1], cp[2]};
    };

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
                .setAmbientColor(Color4{toMagnumColor(mat->getAmbient()) + toMagnumColor(mat->getEmission()) + dyncol, mat->getDiffuse()[3]})
                .setSpecularColor(Color4{toMagnumColor(mat->getSpecular()), 0.0f})
                .setShininess(mat->getShininess())
                .setAlphaMask(mat->getAlphaThreshold())
        });

        if (mat->getNoCulling())
            GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
        else
            GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

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
            _shader.bindDiffuseTexture(*t)
                .bindAmbientTexture(*t)
                .bindLightBuffer(lightUniform)
                .bindProjectionBuffer(projectionUniform)
                .bindMaterialBuffer(materialUniform)
                .bindTransformationBuffer(transformationUniform)
                .bindDrawBuffer(drawUniform)
                .bindTextureTransformationBuffer(textureTransformationUniform)
                .draw(_mesh);
        } else {
            _shaderUntex
                .bindLightBuffer(lightUniform)
                .bindProjectionBuffer(projectionUniform)
                .bindMaterialBuffer(materialUniform)
                .bindTransformationBuffer(transformationUniform)
                .bindDrawBuffer(drawUniform)
                .draw(_mesh);
        }
    } else {
        Warning{} << "Missing mat" << _matName.c_str();
    }
}

void DebugLineDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setColor(_color)
        .setViewportSize(Vector2{GL::defaultFramebuffer.viewport().size()})
        .setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix)
        .setWidth(2.0f)
        .draw(_mesh);
}
