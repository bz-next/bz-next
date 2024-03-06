#ifndef MAGNUMSCENERENDERER_H
#define MAGNUMSCENERENDERER_H

#include "Magnum/SceneGraph/SceneGraph.h"
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/GL/Texture.h>

#include "MagnumDefs.h"

// This class handles rendering the scene
// The scene is specified by some parent scene objects in SceneObjectManager
// Each scene object has a Renderable attached.
// The renderable connects the scene object to a mesh and BZ material
// The renderable uses the current DrawMode to render the scene to the currently
// attached framebuffer.
// This renderer class can set state, manipulate the DrawMode, attach framebuffers
// etc in order to implement fancy rendering.
class MagnumSceneRenderer {
    void renderScene(Magnum::SceneGraph::Camera3D* camera);
    void renderLightDepthMap(Magnum::SceneGraph::Camera3D* lightCamera);
    private:
    Magnum::SceneGraph::Camera3D* _camera;
    Object3D _cameraObject;

     const Magnum::Math::Vector2<int> _depthMapSize{512, 512};
     Magnum::GL::Texture2D _depthMapTex;
};

#endif
