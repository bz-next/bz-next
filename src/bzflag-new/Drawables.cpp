#include "Drawables.h"

#include <Magnum/SceneGraph/Camera.hpp>

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
