#ifndef MAGNUMSCENERENDERER_H
#define MAGNUMSCENERENDERER_H

#include "BasicTexturedShader.h"
#include "DepthMapVisualizerShader.h"
#include "Magnum/Math/Vector3.h"
#include "Magnum/SceneGraph/SceneGraph.h"
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/GL/Texture.h>

#include "MagnumDefs.h"

#include "DrawMode.h"

// For TextureData
#include "MagnumTextureManager.h"

#include "RaymarchedCloudsShader.h"

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
    void renderClouds(Magnum::SceneGraph::Camera3D* camera);
    void renderSceneToHDR(Magnum::SceneGraph::Camera3D* camera);

    // We store render pipeline textures in a map
    // that way, we can access them from anywhere by name
    // and we can also pass a reference to the map elsewhere for
    // rendering imgui preview windows and such
    TextureData getPipelineTex(const std::string& name);
    void addPipelineTex(const std::string& name, TextureData data);

    void setLightObj(Object3D *obj) { _lightObj = obj; }

    void setSunPosition(Magnum::Math::Vector3<float> position);
    Magnum::Math::Vector3<float> getSunPosition() const;

    void resizeViewport(unsigned int w, unsigned int h);

    void setShadowMapTexSize(unsigned int dim);

    // For ImGUI integration
    std::map<std::string, TextureData>& getPipelineTextures() { return _pipelineTexMap; }
    void drawPipelineTexBrowser(const char *title, bool *p_open);
    void drawSettings(const char *title, bool *p_open);
    private:

    Object3D *_lightObj = NULL;

    // For depth map render projection matrix
    // Compute based on world extents
    float getSunNearPlane() const;
    float getSunFarPlane() const;

    bool _enableShadowMapping = true;
    bool _enableClouds = true;

    Magnum::SceneGraph::Camera3D* _lightCamera;
    Object3D _cameraObject;

    Magnum::Math::Vector2<int> _depthMapSize{4096, 4096};

    Magnum::Math::Vector2<int> _viewportSize{512, 512};

    std::map<std::string, TextureData> _pipelineTexMap;

    BZMaterialDrawMode _bzmatMode;
    BZMaterialShadowMappedDrawMode _bzmatShadowMode;
    DepthMapDrawMode _depthMode;

    DepthMapVisualizerShader _depthVisShader;
    BasicTexturedShader _basicTexturedShader;

    RaymarchedCloudsShader _cloudShader;

    Magnum::GL::Mesh _quadMesh;
};

#endif
