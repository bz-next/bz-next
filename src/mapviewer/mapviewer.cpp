#include "Magnum/GL/Context.h"
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/DebugStl.h>
#include <Corrade/Utility/Resource.h>

#include <Magnum/GL/Version.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>

#ifndef TARGET_EMSCRIPTEN
#include <Magnum/Platform/Sdl2Application.h>
#else
#include <Magnum/Platform/EmscriptenApplication.h>
#endif
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Types.h>

#include <imgui.h>

#include "GLInfo.h"

#include "MagnumDefs.h"
#include "MagnumTextureManager.h"
#include "WorldSceneObjectGenerator.h"

#include "BZTextureBrowser.h"
#include "BZMaterialBrowser.h"
#include "BZMaterialViewer.h"
#include "ObstacleBrowser.h"

#include "WorldMeshGenerator.h"

#include <ctime>
#include <cassert>
#include <cstring>

#ifdef TARGET_EMSCRIPTEN
#include "emscripten_browser_file.h"
#else
#include "imfilebrowser.h"
#endif

#include "common.h"

#include "BZDBCache.h"
#include "BZDBLocal.h"
#include "ErrorHandler.h"
#include "GameTime.h"
#include "PhysicsDriver.h"
#include "ObstacleMgr.h"
#include "TextureMatrix.h"
#include "DynamicColor.h"
#include "Teleporter.h"

#include "Downloads.h"

#include "BZWReader.h"

// defaults for bzdb
#include "defaultBZDB.h"

// Some leftover global state that bzflag code needs
int         debugLevel = 4;

class BasicLoggingCallback : public LoggingCallback {
    public:
        void log(int lvl, const char* msg) override;
};

void BasicLoggingCallback::log(int lvl, const char* msg)
{
    if (lvl <= debugLevel)
        std::cout << lvl << " " << msg << std::endl;
}

BasicLoggingCallback blc;

using namespace Magnum;
using namespace Magnum::Math::Literals;

class MapViewer: public Platform::Application {
    public:
        explicit MapViewer(const Arguments& arguments);
        void init();


    private:
        //void tickEvent() override;

        void drawEvent() override;

        void viewportEvent(ViewportEvent& e) override;
        void mousePressEvent(MouseEvent& e) override;
        void mouseReleaseEvent(MouseEvent& e) override;
        void mouseMoveEvent(MouseMoveEvent& e) override;
        void mouseScrollEvent(MouseScrollEvent& e) override;
        void textInputEvent(TextInputEvent& event) override;
        //void exitEvent(ExitEvent& event) override;

        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;

        void tryConnect(const std::string& callsign, const std::string& password, const std::string& server, const std::string& port);

        // IMGUI
        void showMainMenuBar();
        void showMenuView();
        void showMenuTools();
        void showMenuDebug();

        void maybeShowProfiler();
        bool showProfiler = false;

        void maybeShowTMBrowser();
        bool showTMBrowser = false;

        void maybeShowMATBrowser();
        bool showMATBrowser = false;

        void maybeShowMATViewer();
        bool showMATViewer = false;

        void maybeShowObsBrowser();
        bool showObsBrowser = false;

        void maybeShowGLInfo();
        bool showGLInfo = false;

        bool showGrid = false;

#ifndef TARGET_EMSCRIPTEN
        void maybeShowFileBrowser();
#endif
        
        Vector3 positionOnSphere(const Vector2i& position) const;

        static void startupErrorCallback(const char* msg);

#ifdef TARGET_EMSCRIPTEN
        static void handleUploadedMap(const std::string& filename, const std::string& mime_type, std::string_view buffer, void*);
        static volatile bool shouldLoadMap;
        static std::string mapData;
        static std::string mapFilename;
#endif

        void loadMap(std::string path, const std::string& data);

        void loopIteration();

        Scene3D _scene;
        Object3D _manipulator, _cameraObject;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;
        Vector3 _previousPosition;

        WorldSceneObjectGenerator worldSceneObjGen;
        WorldMeshGenerator worldSceneBuilder;
        ImGuiIntegration::Context _imgui{NoCreate};

        BZTextureBrowser tmBrowser;
        BZMaterialBrowser matBrowser;
        BZMaterialViewer matViewer;
        ObstacleBrowser obsBrowser;
#ifndef TARGET_EMSCRIPTEN
        ImGui::FileBrowser fileBrowser;
#endif

        // Ugh... need this for a C-style callback from emscripten
        static MapViewer *instance;
};

MapViewer *MapViewer::instance = NULL;

#ifdef TARGET_EMSCRIPTEN
volatile bool MapViewer::shouldLoadMap = false;
std::string MapViewer::mapData;
std::string MapViewer::mapFilename;
#endif

MapViewer::MapViewer(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("BZFlag Map Viewer")
        .setWindowFlags(Configuration::WindowFlag::Resizable | Configuration::WindowFlag::AlwaysRequestAnimationFrame),
#if defined(MAGNUM_TARGET_GLES) || defined(MAGNUM_TARGET_GLES2)
        // No multisampling for GLES, assume less capable machine
        GLConfiguration{}.setVersion(GL::Version::GLES300)
#else
        GLConfiguration{}.setSampleCount(4)
#endif
        }
{
    using namespace Math::Literals;

    assert(instance == NULL);

    instance = this;

    MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GLES300);

    loggingCallback = &blc;
    debugLevel = 0;

    Utility::Arguments args;
    args
        .setGlobalHelp("BZFlag Map Viewer")
        .parse(arguments.argc, arguments.argv);
    
    _cameraObject
        .setParent(&_scene)
        .translate(Vector3::zAxis(10.0f));
        
    
    (*(_camera = new SceneGraph::Camera3D{_cameraObject}))
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 1.0f, 1000.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());
    
    _manipulator.setParent(&_scene);


    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);

    _imgui = ImGuiIntegration::Context(Vector2{windowSize()}/dpiScaling(), windowSize(), framebufferSize());

#ifndef TARGET_EMSCRIPTEN
    fileBrowser.SetTitle("Select Map File");
    fileBrowser.SetTypeFilters({".bzw"});
#endif

    /* Set up proper blending to be used by ImGui. There's a great chance
       you'll need this exact behavior for the rest of your scene. If not, set
       this only for the drawFrame() call. */
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
        GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    init();
}
/*
void MapViewer::exitEvent(ExitEvent& event) {
    event.setAccepted();
    MagnumTextureManager::instance().clear();
}*/

#ifdef TARGET_EMSCRIPTEN
void MapViewer::handleUploadedMap(const std::string& filename, const std::string& mime_type, std::string_view buffer, void*) {
    mapFilename = filename;
    mapData = std::string(buffer);
    shouldLoadMap = true;
}
#endif

void MapViewer::showMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::MenuItem("Load")) {
#ifdef TARGET_EMSCRIPTEN
            emscripten_browser_file::upload(".bzw", handleUploadedMap, NULL);
#else
            fileBrowser.Open();
#endif
        }
        if (ImGui::BeginMenu("View")) {
            showMenuView();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools")) {
            showMenuTools();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            showMenuDebug();
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void MapViewer::showMenuView() {
#ifndef MAGNUM_TARGET_GLES2
    if (ImGui::MenuItem("Grid", NULL, &showGrid)) {}
#endif
}

void MapViewer::showMenuTools() {
    if (ImGui::MenuItem("Texture Manager", NULL, &showTMBrowser)) {}
    ImGui::Separator();
    if (ImGui::MenuItem("Material Manager", NULL, &showMATBrowser)) {}
    if (ImGui::MenuItem("Material Viewer", NULL, &showMATViewer)) {}
    ImGui::Separator();
    if (ImGui::MenuItem("Obstacle Manager", NULL, &showObsBrowser)) {}
}

void MapViewer::showMenuDebug() {
    if (ImGui::MenuItem("Profiler", NULL, &showProfiler)) {}
    if (ImGui::MenuItem("GL Info", NULL, &showGLInfo)) {}
    ImGui::Separator();
    if (ImGui::MenuItem("Recompile World Mesh")) {
        worldSceneObjGen.destroyWorldObject();
        worldSceneObjGen.createWorldObject(&worldSceneBuilder);
        worldSceneObjGen.getWorldObject()->setParent(&_manipulator);
    }
    if (ImGui::MenuItem("Force Load Material Textures")) {
        MAGNUMMATERIALMGR.forceLoadTextures();
    }

}

void MapViewer::maybeShowProfiler() {
    if (showProfiler) {
        ImGui::Begin("Profiler", &showProfiler);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
            1000.0/Double(ImGui::GetIO().Framerate), Double(ImGui::GetIO().Framerate));
        ImGui::End();
    }
}

void MapViewer::maybeShowTMBrowser()
{
    if (showTMBrowser) {
        tmBrowser.draw("Texture Manager", &showTMBrowser);
    }
}

void MapViewer::maybeShowMATBrowser()
{
    if (showMATBrowser) {
        matBrowser.draw("Material Manager", &showMATBrowser);
    }
}

void MapViewer::maybeShowMATViewer()
{
    if (showMATViewer) {
        matViewer.draw("Material Viewer", &showMATViewer);
    }
}

void MapViewer::maybeShowObsBrowser() {
    if (showObsBrowser) {
        ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
        obsBrowser.draw("Obstacle Manager", &showObsBrowser);
    }
}

void MapViewer::maybeShowGLInfo()
{
    static std::string info = getGLInfo();
    if (showGLInfo) {
        ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
        ImGui::Begin("OpenGL Info", &showGLInfo);
        ImGui::TextWrapped(info.c_str());
        ImGui::End();
    }
}

#ifndef TARGET_EMSCRIPTEN
void MapViewer::maybeShowFileBrowser() {
    // Filebrowser tracks whether to draw internally
    fileBrowser.Display();
    if (fileBrowser.HasSelected()) {
        loadMap(fileBrowser.GetSelected().string(), "");
        fileBrowser.ClearSelected();
    }
}
#endif

void MapViewer::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    _imgui.newFrame();

    if(ImGui::GetIO().WantTextInput && !isTextInputActive())
        startTextInput();
    else if(!ImGui::GetIO().WantTextInput && isTextInputActive())
        stopTextInput();

    showMainMenuBar();
    maybeShowProfiler();
    maybeShowTMBrowser();
    maybeShowMATBrowser();
    maybeShowMATViewer();
    maybeShowObsBrowser();
    maybeShowGLInfo();
#ifndef TARGET_EMSCRIPTEN
    maybeShowFileBrowser();
#endif

    _imgui.updateApplicationCursor(*this);

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    if (showGrid)
        if (auto* dg = worldSceneObjGen.getDebugDrawableGroup())
            _camera->draw(*dg);
    if (auto* dg = worldSceneObjGen.getDrawableGroup())
        _camera->draw(*dg);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    if (auto* dg = worldSceneObjGen.getTransDrawableGroup())
        _camera->draw(*dg);

    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

    _imgui.drawFrame();

    swapBuffers();
    loopIteration();
    redraw();
}

/*
void MapViewer::tickEvent() {
    loopIteration();
    redraw();
}*/

void MapViewer::viewportEvent(ViewportEvent& e) {
    GL::defaultFramebuffer.setViewport({{}, e.framebufferSize()});
    _imgui.relayout(Vector2{e.windowSize()}/e.dpiScaling(),
        e.windowSize(), e.framebufferSize());
    _camera->setViewport(e.windowSize());
}

void MapViewer::keyPressEvent(KeyEvent& event) {
    event.setAccepted();
    if(_imgui.handleKeyPressEvent(event)) return;
}

void MapViewer::keyReleaseEvent(KeyEvent& event) {
    event.setAccepted();
    if(_imgui.handleKeyReleaseEvent(event)) return;
}

void MapViewer::mousePressEvent(MouseEvent& e) {
    e.setAccepted();
    if(_imgui.handleMousePressEvent(e)) return;
    if (e.button() == MouseEvent::Button::Left)
        _previousPosition = positionOnSphere(e.position());
}

void MapViewer::mouseReleaseEvent(MouseEvent& e) {
    e.setAccepted();
    if(_imgui.handleMouseReleaseEvent(e)) return;
    if (e.button() == MouseEvent::Button::Left)
        _previousPosition = Vector3{};
}

void MapViewer::mouseScrollEvent(MouseScrollEvent& e) {
    if(_imgui.handleMouseScrollEvent(e)) {
        /* Prevent scrolling the page */
        e.setAccepted();
        return;
    }
    e.setAccepted();
    if (!e.offset().y()) return;
    const Float distance = _cameraObject.transformation().translation().z();
    _cameraObject.translate(Vector3::zAxis(distance*(1.0f - (e.offset().y() > 0 ? 1/0.85f : 0.85f))));
    redraw();
}

void MapViewer::mouseMoveEvent(MouseMoveEvent &e) {
    e.setAccepted();
    if(_imgui.handleMouseMoveEvent(e)) return;
    if (!(e.buttons() & MouseMoveEvent::Button::Left)) return;

    const Vector3 currentPosition = positionOnSphere(e.position());
    const Vector3 axis = Math::cross(_previousPosition, currentPosition);

    if (_previousPosition.length() < 0.001f || axis.length() < 0.001f) return;

    _manipulator.rotate(Math::angle(_previousPosition, currentPosition), axis.normalized());
    _previousPosition = currentPosition;

    redraw();
}

void MapViewer::textInputEvent(TextInputEvent& e) {
    e.setAccepted();
    if(_imgui.handleTextInputEvent(e)) return;
}

Vector3 MapViewer::positionOnSphere(const Vector2i& position) const {
    const Vector2 positionNormalized = Vector2{position}/Vector2{_camera->viewport()} - Vector2{0.5f};
    const Float length = positionNormalized.length();
    const Vector3 result(length > 1.0f ? Vector3(positionNormalized, 0.0f) : Vector3(positionNormalized, 1.0f - length));
    return (result * Vector3::yScale(-1.0f)).normalized();
}

void MapViewer::init() {
    bzfsrand((unsigned int)time(0));
    // set default DB entries
    for (unsigned int gi = 0; gi < numGlobalDBItems; ++gi)
    {
        assert(globalDBItems[gi].name != NULL);
        if (globalDBItems[gi].value != NULL)
        {
            BZDB.set(globalDBItems[gi].name, globalDBItems[gi].value);
            BZDB.setDefault(globalDBItems[gi].name, globalDBItems[gi].value);
        }
        BZDB.setPersistent(globalDBItems[gi].name, globalDBItems[gi].persistent);
        BZDB.setPermission(globalDBItems[gi].name, globalDBItems[gi].permission);
    }

    BZDBCache::init();
    BZDBLOCAL.init();

    loadBZDBDefaults();

    worldSceneObjGen.createWorldObject(&worldSceneBuilder);
    worldSceneObjGen.getWorldObject()->setParent(&_manipulator);

    setErrorCallback(startupErrorCallback);

    /*Utility::Resource rs{"bundledmaps"};
    if (rs.hasFile("church.bzw")) {
        Warning{} << "Loading churchyard from bundled resources";
        mapFilename = "church.bzw";
        mapData = rs.getRaw(mapFilename);
        shouldLoadMap = true;
    }*/

    //CachedTexture *test = new CachedTexture("https://images.bzflag.org/bz_legacy/louman/churchyard/Brick05.png");

}

void MapViewer::loadMap(std::string path, const std::string& data)
{
    DYNCOLORMGR.clear();
    TEXMATRIXMGR.clear();
    MAGNUMMATERIALMGR.clear(false);
    PHYDRVMGR.clear();
    TRANSFORMMGR.clear();
    OBSTACLEMGR.clear();
    Downloads::removeTextures();
    MagnumTextureManager::instance().clear();

    if (data == "") {
        BZWReader* reader = new BZWReader(path);
        auto worldInfo = reader->defineWorldFromFile();
        delete reader;
    } else {
        BZWReader* reader = new BZWReader(path, data);
        auto worldInfo = reader->defineWorldFromFile();
        delete reader;
    }

    MAGNUMMATERIALMGR.loadDefaultMaterials();

    Downloads::startDownloads(true, true, false);

}

void MapViewer::startupErrorCallback(const char* msg)
{
    Warning{} << msg;
}

void MapViewer::loopIteration()
{
    // set this step game time
    GameTime::setStepTime();

    // update the dynamic colors
    DYNCOLORMGR.update();

    // update the texture matrices
    TEXMATRIXMGR.update();

    if (shouldLoadMap) {
        loadMap(mapFilename, mapData);
        shouldLoadMap = false;
    }

    //CachedTexture *t = new CachedTexture("https://images.bzflag.org/bz_legacy/trepan/tiles/Bad_Road_Condition.png");

    if (Downloads::requestFinalized()) {
        Downloads::finalizeDownloads();
        worldSceneObjGen.destroyWorldObject();
    worldSceneBuilder.reset();

    const ObstacleList& boxes = OBSTACLEMGR.getBoxes();
    for (int i = 0; i < boxes.size(); ++i) {
        worldSceneBuilder.addBox(*((BoxBuilding*) boxes[i]));
    }
    const ObstacleList& pyrs = OBSTACLEMGR.getPyrs();
    for (int i = 0; i < pyrs.size(); ++i) {
        worldSceneBuilder.addPyr(*((PyramidBuilding*) pyrs[i]));
    }
    const ObstacleList& bases = OBSTACLEMGR.getBases();
    for (int i = 0; i < bases.size(); ++i) {
        worldSceneBuilder.addBase(*((BaseBuilding*) bases[i]));
    }
    const ObstacleList& walls = OBSTACLEMGR.getWalls();
    for (int i = 0; i < walls.size(); ++i) {
        worldSceneBuilder.addWall(*((WallObstacle*) walls[i]));
    }
    const ObstacleList& teles = OBSTACLEMGR.getTeles();
    for (int i = 0; i < teles.size(); ++i) {
        worldSceneBuilder.addTeleporter(*((Teleporter*) teles[i]));
    }
    worldSceneBuilder.addGround(BZDBCache::worldSize);
    const ObstacleList& meshes = OBSTACLEMGR.getMeshes();
    for (int i = 0; i < meshes.size(); i++)
        worldSceneBuilder.addMesh (*((MeshObstacle*) meshes[i]));
    
    worldSceneObjGen.createWorldObject(&worldSceneBuilder);
    worldSceneObjGen.getWorldObject()->setParent(&_manipulator);
    }
}

MAGNUM_APPLICATION_MAIN(MapViewer)