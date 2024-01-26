#include "MagnumDefs.h"

#include <Magnum/SceneGraph/SceneGraph.h>
#include <Magnum/SceneGraph/Drawable.hpp>
#include <Magnum/Shaders/Shaders.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/GL/GL.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Mesh.h>
#include <Magnum/Math/Color.h>

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
