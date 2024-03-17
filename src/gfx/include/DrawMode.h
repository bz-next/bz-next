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
        Magnum::GL::Mesh& mesh,
        Object3D* obj) = 0;
};

class BZMaterialDrawMode : public DrawMode {
    public:
    BZMaterialDrawMode();
    void draw(
        const Magnum::Matrix4& transformationMatrix,
        Magnum::SceneGraph::Camera3D& camera,
        const MagnumBZMaterial* mat,
        Magnum::GL::Mesh& mesh,
        Object3D* obj) override;
    void setLightObj(Object3D* obj) { _lightObj = obj; }
    void setLightCamera(Magnum::SceneGraph::Camera3D* c) { _lightCamera = c; }
    private:
    EnhancedPhongGL *_shader;
    EnhancedPhongGL *_shaderUntex;
    Object3D* _lightObj;
    Magnum::SceneGraph::Camera3D* _lightCamera;
};

class BZMaterialShadowMappedDrawMode : public DrawMode {
    public:
    BZMaterialShadowMappedDrawMode();
    void draw(
        const Magnum::Matrix4& transformationMatrix,
        Magnum::SceneGraph::Camera3D& camera,
        const MagnumBZMaterial* mat,
        Magnum::GL::Mesh& mesh,
        Object3D* obj) override;
    void setLightObj(Object3D* obj) { _lightObj = obj; }
    void setLightCamera(Magnum::SceneGraph::Camera3D* c) { _lightCamera = c; }
    void setShadowMap(Magnum::GL::Texture2D* tex) {
        _shadowMapTex = tex;
    }
    private:
    EnhancedPhongGL *_shader;
    EnhancedPhongGL *_shaderUntex;
    Object3D* _lightObj;
    Magnum::GL::Texture2D *_shadowMapTex;
    Magnum::SceneGraph::Camera3D* _lightCamera;
};

class BasicTexturedShaderDrawMode : public DrawMode {
    public:
    BasicTexturedShaderDrawMode();
    void draw(
        const Magnum::Matrix4& transformationMatrix,
        Magnum::SceneGraph::Camera3D& camera,
        const MagnumBZMaterial* mat,
        Magnum::GL::Mesh& mesh,
        Object3D* obj) override;
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
        Magnum::GL::Mesh& mesh,
        Object3D* obj) override;
    private:
    DepthMapShader *_shader;
};

#endif
