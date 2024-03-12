#ifndef DRAWMODE_H
#define DRAWMODE_H

#include "Magnum/SceneGraph/SceneGraph.h"
#include "MagnumBZMaterial.h"
#include <Magnum/Math/Matrix4.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Shaders/PhongGL.h>

#include "BasicTexturedShader.h"
#include "DepthMapShader.h"

#include "MagnumDefs.h"

#include "EnhancedPhongGL.h"

class DrawMode {
    public:
    virtual void draw(
        const Magnum::Matrix4& transformationMatrix,
        Magnum::SceneGraph::Camera3D& camera,
        const MagnumBZMaterial* mat,
        Magnum::GL::Mesh& mesh) = 0;
};

class BZMaterialDrawMode : public DrawMode {
    public:
    BZMaterialDrawMode();
    void draw(
        const Magnum::Matrix4& transformationMatrix,
        Magnum::SceneGraph::Camera3D& camera,
        const MagnumBZMaterial* mat,
        Magnum::GL::Mesh& mesh) override;
    void setLightObj(Object3D* obj) { _lightObj = obj; }
    private:
    EnhancedPhongGL *_shader;
    EnhancedPhongGL *_shaderUntex;
    Object3D* _lightObj;
};

class BasicTexturedShaderDrawMode : public DrawMode {
    public:
    BasicTexturedShaderDrawMode();
    void draw(
        const Magnum::Matrix4& transformationMatrix,
        Magnum::SceneGraph::Camera3D& camera,
        const MagnumBZMaterial* mat,
        Magnum::GL::Mesh& mesh) override;
    private:
    BasicTexturedShader *_shader;
};

class DepthMapDrawMode : public DrawMode {
    public:
    DepthMapDrawMode();
    void draw(
        const Magnum::Matrix4& transformationMatrix,
        Magnum::SceneGraph::Camera3D& camera,
        const MagnumBZMaterial* mat,
        Magnum::GL::Mesh& mesh) override;
    private:
    DepthMapShader *_shader;
};

#endif
