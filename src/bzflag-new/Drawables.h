#include "MagnumDefs.h"

#include <Magnum/SceneGraph/SceneGraph.h>
#include <Magnum/SceneGraph/Drawable.hpp>
#include <Magnum/Shaders/Shaders.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/GL/GL.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Shaders/LineGL.h>

#include "MagnumBZMaterial.h"

class ColoredDrawable : public Magnum::SceneGraph::Drawable3D {
    public:
        explicit ColoredDrawable(Object3D& object, Magnum::Shaders::PhongGL& shader, Magnum::GL::Mesh& mesh, const Magnum::Color4& color, Magnum::SceneGraph::DrawableGroup3D& group) :
            Magnum::SceneGraph::Drawable3D{object, &group},
            _shader(shader),
            _mesh(mesh),
            _color(color)
        {}

    private:
        void draw(const Magnum::Matrix4& transformationMatrix, Magnum::SceneGraph::Camera3D& camera) override;

        Magnum::Shaders::PhongGL& _shader;
        Magnum::GL::Mesh& _mesh;
        Magnum::Color4 _color;
};

class InstancedColoredDrawable : public Magnum::SceneGraph::Drawable3D {
    public:
        explicit InstancedColoredDrawable(Object3D& object, Magnum::Shaders::PhongGL& shader, Magnum::GL::Mesh& mesh, Magnum::SceneGraph::DrawableGroup3D& group) :
            Magnum::SceneGraph::Drawable3D{object, &group},
            _shader(shader),
            _mesh(mesh)
        {}

    private:
        void draw(const Magnum::Matrix4& transformationMatrix, Magnum::SceneGraph::Camera3D& camera) override;

        Magnum::Shaders::PhongGL& _shader;
        Magnum::GL::Mesh& _mesh;
};

class TexturedDrawable : public Magnum::SceneGraph::Drawable3D {
    public:
        explicit TexturedDrawable(Object3D& object, Magnum::Shaders::PhongGL& shader, Magnum::GL::Mesh& mesh, Magnum::GL::Texture2D& texture, Magnum::SceneGraph::DrawableGroup3D& group) :
            Magnum::SceneGraph::Drawable3D{object, &group},
            _shader(shader),
            _mesh(mesh),
            _texture(texture)
        {}

    private:
        void draw(const Magnum::Matrix4& transformationMatrix, Magnum::SceneGraph::Camera3D& camera) override;

        Magnum::Shaders::PhongGL& _shader;
        Magnum::GL::Mesh& _mesh;
        Magnum::GL::Texture2D &_texture;
};

class BZMaterialDrawable : public Magnum::SceneGraph::Drawable3D {
    public:
        explicit BZMaterialDrawable(Object3D& object, Magnum::Shaders::PhongGL& shader, Magnum::Shaders::PhongGL& shaderUntex, Magnum::GL::Mesh& mesh, std::string matname, Magnum::SceneGraph::DrawableGroup3D& group, const MagnumBZMaterial* mptr) :
            Magnum::SceneGraph::Drawable3D{object, &group},
            _shader(shader),
            _shaderUntex(shaderUntex),
            _mesh(mesh),
            _matName(matname),
            _matPtr(mptr)
        {}

    private:
        void draw(const Magnum::Matrix4& transformationMatrix, Magnum::SceneGraph::Camera3D& camera) override;

        Magnum::Shaders::PhongGL &_shader;
        Magnum::Shaders::PhongGL &_shaderUntex;
        Magnum::GL::Mesh& _mesh;
        std::string _matName;
        const MagnumBZMaterial *_matPtr;
};

class DebugLineDrawable : public Magnum::SceneGraph::Drawable3D {
    public:
        explicit DebugLineDrawable(Object3D& object, Magnum::Shaders::LineGL3D& shader, const Magnum::Color3& color, Magnum::GL::Mesh& mesh, Magnum::SceneGraph::DrawableGroup3D& group) :
            Magnum::SceneGraph::Drawable3D{object, &group},
            _shader(shader),
            _mesh(mesh),
            _color(color)
        {}

    private:
        void draw(const Magnum::Matrix4& transformationMatrix, Magnum::SceneGraph::Camera3D& camera) override;

        Magnum::Shaders::LineGL3D &_shader;
        Magnum::GL::Mesh& _mesh;
        Magnum::Color3 _color;
        std::string _matName;
};