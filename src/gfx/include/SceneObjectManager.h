#ifndef SCENEOBJECTMANAGER_H
#define SCENEOBJECTMANAGER_H

#include "Magnum/SceneGraph/SceneGraph.h"
#include <map>
#include "MagnumDefs.h"

// Keeps track of references to scenegraph objects that have a long lifetime
// Such as the world parent
class SceneObjectManager {
    public:
    SceneObjectManager() {}
    Object3D* getObj(const std::string& name);
    Object3D* addObj(const std::string& name);
    void delObj(const std::string& name);
    private:
    std::map<std::string, Object3D*> _objs;
};

extern SceneObjectManager SOMGR;

#endif
