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

#include "WorldSceneBuilder.h"

#include <map>
#include <list>

class WorldRenderer {
    public:
        WorldRenderer();
        ~WorldRenderer();
        // Assumes that world is already loaded and OBSTACLEMGR is ready to go
        void createWorldObject();
        void test(const WorldSceneBuilder *sb);

        Magnum::SceneGraph::DrawableGroup3D *getDrawableGroup();
        Object3D *getWorldObject();

        void destroyWorldObject();
    private:
        struct InstanceData {
            Magnum::Matrix4 transformationMatrix;
            Magnum::Matrix3x3 normalMatrix;
            Magnum::Color3 color;
        };
        std::map<std::string, std::list<Magnum::GL::Mesh>> worldMeshes;
        Object3D *worldParent;
        Magnum::SceneGraph::DrawableGroup3D *worldDrawables;
        Magnum::Shaders::PhongGL coloredShader;
        Magnum::Shaders::PhongGL coloredShaderInstanced{Magnum::Shaders::PhongGL::Configuration{}
            .setFlags(Magnum::Shaders::PhongGL::Flag::InstancedTransformation|
                    Magnum::Shaders::PhongGL::Flag::VertexColor)};
        Magnum::Shaders::PhongGL matShader{Magnum::Shaders::PhongGL::Configuration{}
            .setFlags(
                Magnum::Shaders::PhongGL::Flag::DiffuseTexture |
                Magnum::Shaders::PhongGL::Flag::AmbientTexture |
                Magnum::Shaders::PhongGL::Flag::UniformBuffers |
                Magnum::Shaders::PhongGL::Flag::AlphaMask |
                Magnum::Shaders::PhongGL::Flag::TextureTransformation)
            .setMaterialCount(1)
            .setLightCount(1)
            .setDrawCount(1)};
        Magnum::Shaders::PhongGL matShaderUntex{Magnum::Shaders::PhongGL::Configuration{}
            .setFlags(
                Magnum::Shaders::PhongGL::Flag::UniformBuffers |
                Magnum::Shaders::PhongGL::Flag::AlphaMask)
            .setMaterialCount(1)
            .setLightCount(1)
            .setDrawCount(1)};
};
