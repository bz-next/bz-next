#include "MagnumDefs.h"

#include "Magnum/GL/GL.h"
#include "Magnum/SceneGraph/SceneGraph.h"

#include <Magnum/Mesh.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/PhongGL.h>
#ifndef MAGNUM_TARGET_GLES2
#include <Magnum/Shaders/LineGL.h>
#endif

#include "WorldMeshGenerator.h"

#include <map>
#include <list>
#include <set>

class WorldSceneObjectGenerator {
    public:
        WorldSceneObjectGenerator();
        ~WorldSceneObjectGenerator();
        // Assumes that world is already loaded and OBSTACLEMGR is ready to go
        void createWorldObject(const WorldMeshGenerator *sb);

        Magnum::SceneGraph::DrawableGroup3D *getDrawableGroup();
        Magnum::SceneGraph::DrawableGroup3D *getTransDrawableGroup();
        Magnum::SceneGraph::DrawableGroup3D *getDebugDrawableGroup();
        Object3D *getWorldObject();

        // Specify materials to exclude when creating world object
        void setExcludeSet(std::set<std::string> matnames);
        void clearExcludeSet();

        void destroyWorldObject();
    private:
        std::set<std::string> materialsToExclude;
        struct InstanceData {
            Magnum::Matrix4 transformationMatrix;
            Magnum::Matrix3x3 normalMatrix;
            Magnum::Color3 color;
        };
        std::map<std::string, std::vector<Magnum::GL::Mesh>> worldMeshes;
        Object3D *worldParent;
        Magnum::GL::Mesh *debugLine;
        Magnum::SceneGraph::DrawableGroup3D *worldDebugDrawables;
        Magnum::SceneGraph::DrawableGroup3D *worldDrawables;
        Magnum::SceneGraph::DrawableGroup3D *worldTransDrawables;
        Magnum::Shaders::PhongGL coloredShader;
        Magnum::Shaders::PhongGL coloredShaderInstanced{Magnum::Shaders::PhongGL::Configuration{}
            .setFlags(Magnum::Shaders::PhongGL::Flag::InstancedTransformation|
                    Magnum::Shaders::PhongGL::Flag::VertexColor)};
        Magnum::Shaders::PhongGL matShader{Magnum::Shaders::PhongGL::Configuration{}
            .setFlags(
                Magnum::Shaders::PhongGL::Flag::DiffuseTexture |
                Magnum::Shaders::PhongGL::Flag::AmbientTexture |
                Magnum::Shaders::PhongGL::Flag::AlphaMask |
                Magnum::Shaders::PhongGL::Flag::TextureTransformation)};
        Magnum::Shaders::PhongGL matShaderUntex{Magnum::Shaders::PhongGL::Configuration{}};
#ifndef MAGNUM_TARGET_GLES2
        Magnum::Shaders::LineGL3D _lineShader;
#endif
};
