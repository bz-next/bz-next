#include "MagnumDefs.h"

#include <Magnum/SceneGraph/SceneGraph.h>
#include <Magnum/SceneGraph/Drawable.hpp>
#include <Magnum/Shaders/Shaders.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/GL/GL.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Mesh.h>
#include <Magnum/Math/Color.h>

#ifndef MAGNUM_TARGET_GLES2
#include <Magnum/Shaders/LineGL.h>
#endif

#include "MagnumBZMaterial.h"

class BZMaterialDrawable : public Magnum::SceneGraph::Drawable3D {
    public:
        explicit BZMaterialDrawable(Object3D& object, Magnum::GL::Mesh& mesh, const MagnumBZMaterial* mptr, Magnum::SceneGraph::DrawableGroup3D& group) :
            Magnum::SceneGraph::Drawable3D{object, &group},
            _mesh(mesh),
            _matPtr(mptr)
        {
            // Quick and dirty way to share a shader among all the bzmaterialdrawables
            if (_shader == NULL)
                _shader = new Magnum::Shaders::PhongGL{Magnum::Shaders::PhongGL::Configuration{}
                    .setFlags(
                        Magnum::Shaders::PhongGL::Flag::DiffuseTexture |
                        Magnum::Shaders::PhongGL::Flag::AmbientTexture |
                        Magnum::Shaders::PhongGL::Flag::AlphaMask |
                        Magnum::Shaders::PhongGL::Flag::TextureTransformation)};
            if (_shaderUntex == NULL)
                _shaderUntex = new Magnum::Shaders::PhongGL{Magnum::Shaders::PhongGL::Configuration{}};
        }

    private:
        void draw(const Magnum::Matrix4& transformationMatrix, Magnum::SceneGraph::Camera3D& camera) override;

        static Magnum::Shaders::PhongGL *_shader;
        static Magnum::Shaders::PhongGL *_shaderUntex;
        Magnum::GL::Mesh& _mesh;
        const MagnumBZMaterial *_matPtr;
};
#ifndef MAGNUM_TARGET_GLES2
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
#endif