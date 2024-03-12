#ifndef MAGNUMSCENERENDERER_H
#define MAGNUMSCENERENDERER_H

#include "BasicTexturedShader.h"
#include "DepthMapVisualizerShader.h"
#include "Magnum/SceneGraph/SceneGraph.h"
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/GL/Texture.h>

#include "MagnumDefs.h"

#include "DrawMode.h"

// For TextureData
#include "MagnumTextureManager.h"

#include <map>

// This class handles rendering the scene
// The scene is specified by some parent scene objects in SceneObjectManager
// Each scene object has a Renderable attached.
// The renderable connects the scene object to a mesh and BZ material
// The renderable uses the current DrawMode to render the scene to the currently
// attached framebuffer.
// This renderer class can set state, manipulate the DrawMode, attach framebuffers
// etc in order to implement fancy rendering.
class MagnumSceneRenderer {
    public:
    MagnumSceneRenderer();
    void init();
    void renderScene(Magnum::SceneGraph::Camera3D* camera);
    void renderLightDepthMap();
    void renderLightDepthMapPreview();

    // We store render pipeline textures in a map
    // that way, we can access them from anywhere by name
    // and we can also pass a reference to the map elsewhere for
    // rendering imgui preview windows and such
    TextureData getPipelineTex(const std::string& name);
    void addPipelineTex(const std::string& name, TextureData data);

    void setLightObj(Object3D *obj) { _lightObj = obj; }

    static Object3D *_lightObj;

    // For ImGUI integration
    std::map<std::string, TextureData>& getPipelineTextures() { return _pipelineTexMap; }
    private:
    Magnum::SceneGraph::Camera3D* _lightCamera;
    Object3D _cameraObject;

    const Magnum::Math::Vector2<int> _depthMapSize{2048, 2048};
    Magnum::GL::Texture2D _depthMapTex;


    std::map<std::string, TextureData> _pipelineTexMap;

    BZMaterialDrawMode _bzmatMode;
    DepthMapDrawMode _depthMode;

    DepthMapVisualizerShader _depthVisShader;
    BasicTexturedShader _basicTexturedShader;

    Magnum::GL::Mesh _quadMesh;
};

#endif
