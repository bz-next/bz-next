#ifndef DRAWMODE_H
#define DRAWMODE_H

#include "Magnum/SceneGraph/SceneGraph.h"
#include "MagnumBZMaterial.h"
#include <Magnum/Math/Matrix4.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Shaders/PhongGL.h>

#include "BasicTexturedShader.h"

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
    private:
    Magnum::Shaders::PhongGL *_shader;
    Magnum::Shaders::PhongGL *_shaderUntex;
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

#endif
