#include "SceneObjectManager.h"
#include "Magnum/SceneGraph/SceneGraph.h"

#include <string>
#include <utility>

using namespace Magnum;

SceneObjectManager SOMGR;

Object3D* SceneObjectManager::getObj(const std::string& name) {
    auto it = _objs.find(name);
    if (it != _objs.end()) {
        return it->second;
    }
    return NULL;
}

Object3D* SceneObjectManager::addObj(const std::string& name) {
    auto o = getObj(name);
    if (o) return o;
    Object3D *obj = new Object3D{};
    _objs.insert(std::make_pair(name, obj));
    return obj;
}

void SceneObjectManager::delObj(const std::string& name) {
    auto it = _objs.find(name);
    if (it != _objs.end()) {
        delete it->second;
        _objs.erase(it);
    }
}