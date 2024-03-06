#include "DrawableGroupManager.h"
#include "Magnum/SceneGraph/SceneGraph.h"

#include <string>
#include <utility>

using namespace Magnum;

DrawableGroupManager DGRPMGR;

SceneGraph::DrawableGroup3D* DrawableGroupManager::getGroup(const std::string& name) {
    auto it = _dgrps.find(name);
    if (it != _dgrps.end()) {
        return it->second;
    }
    return NULL;
}

SceneGraph::DrawableGroup3D* DrawableGroupManager::addGroup(const std::string& name) {
    auto g = getGroup(name);
    if (g != NULL) return g;
    _dgrps.insert(std::make_pair(name, new SceneGraph::DrawableGroup3D{}));
    return _dgrps[name];
}

bool DrawableGroupManager::deleteGroup(const std::string& name) {
    auto it = _dgrps.find(name);
    if (it != _dgrps.end()) {
        delete it->second;
        _dgrps.erase(it);
        return true;
    }
    return false;
}