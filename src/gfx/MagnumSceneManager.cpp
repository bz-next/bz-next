#include "MagnumSceneManager.h"
#include "SceneObjectManager.h"
#include "DrawableGroupManager.h"

#include <string>

void MagnumSceneManager::initScene() {
    // Init scene objects
    // Scene root
    Object3D* scene = SOMGR.getObj("Scene");
    // World parent
    Object3D* world = SOMGR.addObj("World");
    world->setParent(scene);
    world->scale({0.05f, 0.05f, 0.05f});
    // Sun
    Object3D* sun = SOMGR.addObj("Sun");
    sun->setParent(world);
    // Tanks parent
    Object3D* tanksParent = SOMGR.addObj("TanksParent");
    tanksParent->setParent(world);

    // Init drawable groups
    DGRPMGR.addGroup("TankDrawables");
    DGRPMGR.addGroup("WorldDrawables");
    DGRPMGR.addGroup("WorldTransDrawables");
    DGRPMGR.addGroup("WorldDebugDrawables");


}
