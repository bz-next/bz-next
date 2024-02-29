#ifndef DRAWABLEGROUPMANAGER_H
#define DRAWABLEGROUPMANAGER_H

#include "Magnum/SceneGraph/SceneGraph.h"
#include <map>
#include <Magnum/SceneGraph/Drawable.h>

class DrawableGroupManager {
    public:
    DrawableGroupManager() {}
    Magnum::SceneGraph::DrawableGroup3D* getGroup(const std::string& name);
    Magnum::SceneGraph::DrawableGroup3D* addGroup(const std::string& name);
    private:
    std::map<std::string, Magnum::SceneGraph::DrawableGroup3D> _dgrps;
};

extern DrawableGroupManager DGRPMGR;

#endif
