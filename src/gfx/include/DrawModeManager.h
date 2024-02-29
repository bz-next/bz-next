#ifndef DRAWMODEMANAGER_H
#define DRAWMODEMANAGER_H

// A basic class to handle muxing of shaders
// So that, for instance, we can switch over to rendering a depth map
// without having to create new drawables and drawable groups just for that purpose.
// (Since scene object creation is necessarily spread all over the code)
// Instead, the main drawable for scene objects grabs a strategy from this manager
// to do its rendering.
class DrawModeManager {
    public:
    DrawModeManager() {}
    
};

extern DrawModeManager DRAWMODEMGR;

#endif
