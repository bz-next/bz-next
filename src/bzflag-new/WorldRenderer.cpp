#include "WorldRenderer.h"
#include "Drawables.h"
#include "WorldPrimitiveGenerator.h"

#include "Obstacle.h"
#include "ObstacleList.h"
#include "ObstacleMgr.h"

#include "Corrade/Containers/ArrayView.h"
#include <Magnum/Trade/MeshData.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/MeshTools/RemoveDuplicates.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/PhongGL.h>

using namespace Magnum;
using namespace Magnum::Math::Literals;

SceneGraph::DrawableGroup3D *WorldRenderer::getDrawableGroup()
{
    return worldDrawables;
}

Object3D *WorldRenderer::getWorldObject()
{
    return worldParent;
}

WorldRenderer::WorldRenderer() {
    worldDrawables = NULL;
    worldParent = NULL;
}

WorldRenderer::~WorldRenderer() {
    if (worldDrawables) delete worldDrawables;
    if (worldParent) delete worldParent;
}

void WorldRenderer::createWorldObject() {
    worldDrawables = new SceneGraph::DrawableGroup3D{};
    worldParent = new Object3D{};
    worldParent->scale({0.005, 0.005, 0.005});
    // Perhaps do culling here in the future if necessary?
    // Could construct instance buffer per-frame.
    auto buildInstanceVectorFromList = [](const ObstacleList &l, Color3 color) {
        std::vector<InstanceData> instances;
        for (int i = 0; i < l.size(); ++i) {
            Object3D obj;
            InstanceData thisInstance;
            
            const float *pos = l[i]->getPosition();
            const float *sz = l[i]->getSize();

            obj.rotateZ(Rad(l[i]->getRotation()));
            obj.translate(Vector3{pos[0], pos[1], pos[2]});
            obj.scaleLocal(Vector3{sz[0], sz[1], sz[2]});

            thisInstance.transformationMatrix = obj.transformationMatrix();
            thisInstance.normalMatrix = obj.transformationMatrix().normalMatrix();

            thisInstance.color = color;
            instances.push_back(thisInstance);
        }
        return instances;
    };
    // Create world box instances (using instanced rendering for performance)
    {
        Object3D *worldCubes = new Object3D;

        worldMeshes["instances"].push_back(MeshTools::compile(Primitives::cubeSolid()));
        GL::Mesh *worldBoxMesh = &worldMeshes["instances"].back();

        std::vector<InstanceData> instances = buildInstanceVectorFromList(OBSTACLEMGR.getBoxes(), 0xCC5511_rgbf);

        worldCubes->setParent(worldParent);
        worldBoxMesh->addVertexBufferInstanced(GL::Buffer{instances}, 1, 0,
            Shaders::PhongGL::TransformationMatrix{},
            Shaders::PhongGL::NormalMatrix{},
            Shaders::PhongGL::Color3{});
        worldBoxMesh->setInstanceCount(instances.size());
        new InstancedColoredDrawable(*worldCubes, coloredShaderInstanced, *worldBoxMesh, *worldDrawables);
    }
    // Create world pyramids
    {
        const ObstacleList& l = OBSTACLEMGR.getPyrs();
        for (int i = 0; i < l.size(); ++i) {
            Object3D *worldPyrs = new Object3D;

            worldMeshes["instances"].push_back(MeshTools::compile(WorldPrimitiveGenerator::pyrSolid()));
            GL::Mesh *worldPyrMesh = &worldMeshes["instances"].back();

            std::vector<InstanceData> instances = buildInstanceVectorFromList(OBSTACLEMGR.getPyrs(), 0x1155CC_rgbf);

            worldPyrs->setParent(worldParent);
            worldPyrMesh->addVertexBufferInstanced(GL::Buffer{instances}, 1, 0,
                Shaders::PhongGL::TransformationMatrix{},
                Shaders::PhongGL::NormalMatrix{},
                Shaders::PhongGL::Color3{});
            worldPyrMesh->setInstanceCount(instances.size());
            new InstancedColoredDrawable(*worldPyrs, coloredShaderInstanced, *worldPyrMesh, *worldDrawables);
        }
    }
}

void WorldRenderer::destroyWorldObject() {
    if (worldDrawables) delete worldDrawables;
    if (worldParent) delete worldParent;
    worldDrawables = NULL;
    worldParent = NULL;
}