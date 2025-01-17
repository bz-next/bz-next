#include "Drawables.h"
#include "DrawMode.h"

#include <Magnum/SceneGraph/Camera.hpp>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include "DrawModeManager.h"

using namespace Magnum;

void BZMaterialDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    //if (_mode == NULL) _mode = new BZMaterialDrawMode();
    DRAWMODEMGR.getDrawMode()->draw(transformationMatrix, camera, _matPtr, _mesh, (Object3D*)&object());
}

#ifndef MAGNUM_TARGET_GLES2
void DebugLineDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setColor(_color)
        .setViewportSize(Vector2{GL::defaultFramebuffer.viewport().size()})
        .setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix)
        .setWidth(2.0f)
        .draw(_mesh);
}
#endif