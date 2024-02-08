/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023 — Vladimír Vondruš <mosra@centrum.cz>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#include "Corrade/Containers/ArrayView.h"
#include "Magnum/DebugTools/FrameProfiler.h"
#include "Magnum/DebugTools/Profiler.h"
#include "Magnum/GL/GL.h"
#include "Magnum/MeshTools/Copy.h"
#include "Magnum/MeshTools/GenerateNormals.h"
#include "Magnum/SceneGraph/SceneGraph.h"
#include "Magnum/Trade/Data.h"
#include "Magnum/Trade/MaterialData.h"
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/DebugStl.h>

#include <Magnum/GL/Version.h>
#include <Magnum/ImageView.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/Duplicate.h>
#include <Magnum/MeshTools/RemoveDuplicates.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/Trade/PhongMaterialData.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Trade/TextureData.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/DebugTools/Profiler.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Types.h>

#include "GLInfo.h"

#include "MagnumBZMaterial.h"
#include "MagnumDefs.h"
#include "PyramidBuilding.h"
#include "WallObstacle.h"
#include "MeshObstacle.h"
#include "WorldRenderer.h"

#include "BZChatConsole.h"
#include "BZScoreboard.h"

#include "MagnumTextureManager.h"

#include "BZTextureBrowser.h"
#include "BZMaterialBrowser.h"
#include "BZMaterialViewer.h"
#include "ObstacleBrowser.h"

#include "WorldSceneBuilder.h"

#include <ctime>
#include <cassert>
#include <imgui.h>
#include <sstream>
#include <cstring>
#include <functional>

#include "utime.h"

#include "common.h"

#include "clientConfig.h"
#include "Address.h"
#include "StartupInfo.h"
#include "StateDatabase.h"
#include "playing.h"
#include "BZDBCache.h"
#include "BZDBLocal.h"
#include "CommandsStandard.h"
#include "ServerListCache.h"
#include "cURLManager.h"
#include "AresHandler.h"
#include "TimeKeeper.h"
#include "commands.h"
#include "CommandManager.h"
#include "ErrorHandler.h"
#include "World.h"
#include "bzfio.h"
#include "version.h"
#include "motd.h"
#include "common.h"
#include "GameTime.h"
#include "ServerList.h"
#include "AccessList.h"
#include "Flag.h"
#include "Roster.h"
#include "Roaming.h"
#include "WordFilter.h"
#include "PlatformFactory.h"
#include "DirectoryNames.h"
#include "BzfMedia.h"
#include "WorldBuilder.h"
#include "Downloads.h"
#include "FileManager.h"
#include "md5.h"
#include "AnsiCodes.h"
#include "PhysicsDriver.h"
#include "ObstacleMgr.h"
#include "TextureMatrix.h"
#include "DynamicColor.h"
#include "Teleporter.h"

// defaults for bzdb
#include "defaultBZDB.h"

bool            echoToConsole = false;
bool            echoAnsi = false;
int         debugLevel = 0;
std::string     alternateConfig;
struct tm       userTime;
const char*     argv0;

class BasicLoggingCallback : public LoggingCallback {
    public:
        void log(int lvl, const char* msg) override;
};

void BasicLoggingCallback::log(int lvl, const char* msg)
{
    std::cout << lvl << " " << msg << std::endl;
}

BasicLoggingCallback blc;

class WorldDownLoader;

void dumpResources()
{
}

using namespace Magnum;
using namespace Magnum::Math::Literals;

class MapViewer: public Platform::Sdl2Application {
    public:
        explicit MapViewer(const Arguments& arguments);
        int main();


    private:
        void tickEvent() override;

        void drawEvent() override;

        void viewportEvent(ViewportEvent& e) override;
        void mousePressEvent(MouseEvent& e) override;
        void mouseReleaseEvent(MouseEvent& e) override;
        void mouseMoveEvent(MouseMoveEvent& e) override;
        void mouseScrollEvent(MouseScrollEvent& e) override;
        void textInputEvent(TextInputEvent& event) override;

        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;

        void exitEvent(ExitEvent& event) override;

        void tryConnect(const std::string& callsign, const std::string& password, const std::string& server, const std::string& port);

        // IMGUI
        void showMainMenuBar();
        void showMenuView();
        void showMenuTools();
        void showMenuDebug();

        void maybeShowConsole();
        bool showConsole = false;

        void maybeShowProfiler();
        bool showProfiler = false;

        void maybeShowScoreboard();
        bool showScoreboard = false;

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

        void maybeShowMatExclude();
        bool showMatExclude = false;

        bool showGrid = false;
        
        Vector3 positionOnSphere(const Vector2i& position) const;

        friend class WorldDownLoader;
        void startPlaying();
        void playingLoop();

        void onConsoleText(const char* txt);

        static void startupErrorCallback(const char* msg);

        void joinInternetGame();
        void joinInternetGame2();

        void leaveGame();

        void setTankFlags();
        void updateFlag(FlagType *flag);

        void doMotion();

        void enteringServer(const void *buf);

        void checkEnvironment();

        void updateFlags(float dt);

        bool isCached(char *hexDigest);
        void loadCachedWorld();

        const void *handleMsgSetVars(const void *msg);

        void markOld(std::string &fileName);

        bool processWorldChunk(const void *buf, uint16_t len, int bytesLeft);

        static void resetServerVar(const std::string& name, void*);

        // Here there be dragons... BZFlag has a lot of global state.
        // I've tried to toss as much as possible under the application object,
        // but there are still some important global objects, like ::World()
        // Managing the state of even these objects is a mess. A lot of this
        // needs to be encapsulated. Expect bugs...
        WorldDownLoader *worldDownLoader;
        double lastObserverUpdateTime = -1;
        StartupInfo startupInfo;
        MessageOfTheDay *motd;
        bool joinRequested = false;
        bool justJoined = false;
        bool serverDied = false;
        bool serverError = false;
        bool joiningGame = false;
        bool entered = false;
        Team *teams = NULL;
        WorldBuilder *worldBuilder = NULL;
        std::string worldUrl;
        bool isCacheTemp = false;
        std::string md5Digest;
        uint32_t worldPtr = 0;
        bool downloadingInitialTexture = false;
        std::string worldCachePath;
        bool firstLife = false;
        bool fireButton = false;
        //ShotStats *shotStats = NULL;
        bool wasRabbit = false;
        char *worldDatabase = NULL;
        std::ostream *cacheOut = NULL;

        DebugTools::FrameProfilerGL _profiler{DebugTools::FrameProfilerGL::Value::FrameTime|
    DebugTools::FrameProfilerGL::Value::GpuDuration, 50};

        std::vector<PlayingCallbackItem> playingCallbacks;

        AccessList ServerAccessList;

        World *world = NULL;

        Shaders::PhongGL _coloredShader;
        Shaders::PhongGL _texturedShader{Shaders::PhongGL::Configuration().setFlags(Shaders::PhongGL::Flag::DiffuseTexture)};
        Containers::Array<Containers::Optional<GL::Mesh>> _meshes;
        Containers::Array<Containers::Optional<GL::Texture2D>> _textures;

        Scene3D _scene;
        Object3D _manipulator, _cameraObject;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;
        Vector3 _previousPosition;

        std::vector<Object3D *> tankObjs;

        WorldRenderer worldRenderer;
        WorldSceneBuilder worldSceneBuilder;
        ImGuiIntegration::Context _imgui{NoCreate};

        BZChatConsole console;
        BZScoreboard scoreboard;
        BZTextureBrowser tmBrowser;
        BZMaterialBrowser matBrowser;
        BZMaterialViewer matViewer;
        ObstacleBrowser obsBrowser;

        bool isQuit = false;
};

MapViewer::MapViewer(const Arguments& arguments):
    Platform::Sdl2Application{arguments, Configuration{}
        .setTitle("BZFlag Experimental Client")
        .setWindowFlags(Configuration::WindowFlag::Resizable),
#if defined(MAGNUM_TARGET_GLES) || defined(MAGNUM_TARGET_GLES2)
        // No multisampling for GLES, assume less capable machine
        GLConfiguration{}.setVersion(GL::Version::GLES200)
#else
        GLConfiguration{}.setSampleCount(4)
#endif
        },
    motd(NULL),
    ServerAccessList("ServerAccess.txt", NULL)
{
    using namespace Math::Literals;

    loggingCallback = &blc;
    debugLevel = 4;

    Utility::Arguments args;
    args/*.addArgument("file").setHelp("file", "file to load")*/
        .addOption("importer", "AnySceneImporter").setHelp("importer", "importer plugin to use")
        .addSkippedPrefix("magnum", "engine-specific options")
        .setGlobalHelp("Displays a 3D scene file provided on command line.")
        .parse(arguments.argc, arguments.argv);
    
    _cameraObject
        .setParent(&_scene)
        .translate(Vector3::zAxis(10.0f));
        
    
    (*(_camera = new SceneGraph::Camera3D{_cameraObject}))
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.1f, 1000.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());
    
    _manipulator.setParent(&_scene);


    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);

    _coloredShader
        .setAmbientColor(0x111111_rgbf)
        .setSpecularColor(0xffffff_rgbf)
        .setShininess(80.0f);
    _texturedShader
        .setAmbientColor(0x111111_rgbf)
        .setSpecularColor(0x111111_rgbf)
        .setShininess(80.0f);

    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer = manager.loadAndInstantiate(args.value("importer"));

    _textures = Containers::Array<Containers::Optional<GL::Texture2D>>{};

    Containers::Array<Containers::Optional<Trade::PhongMaterialData>> materials{};

    _meshes = Containers::Array<Containers::Optional<GL::Mesh>>{1};

    //worldRenderer.getWorldObject()->setParent(&_manipulator);

    _imgui = ImGuiIntegration::Context(Vector2{windowSize()}/dpiScaling(), windowSize(), framebufferSize());

    console.registerCommandCallback([&](const char* txt){
        onConsoleText(txt);
    });

    /* Set up proper blending to be used by ImGui. There's a great chance
       you'll need this exact behavior for the rest of your scene. If not, set
       this only for the drawFrame() call. */
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
        GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    setMinimalLoopPeriod(0);
    //setSwapInterval(0);
}

void MapViewer::showMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
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
    if (ImGui::MenuItem("Scoreboard", NULL, &showScoreboard)) {}
    if (ImGui::MenuItem("Console", NULL, &showConsole)) {}
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
        worldRenderer.destroyWorldObject();
        worldRenderer.createWorldObject(&worldSceneBuilder);
        worldRenderer.getWorldObject()->setParent(&_manipulator);
    }
    if (ImGui::MenuItem("Force Load Material Textures")) {
        MAGNUMMATERIALMGR.forceLoadTextures();
    }
    if (ImGui::MenuItem("Exclude Materials from World", NULL, &showMatExclude)) {
    }

}

void MapViewer::maybeShowConsole() {
    if (showConsole)
        console.draw("Console", &showConsole);
}

void MapViewer::maybeShowProfiler() {
    if (showProfiler) {
        ImGui::Begin("Profiler", &showProfiler);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
            1000.0/Double(ImGui::GetIO().Framerate), Double(ImGui::GetIO().Framerate));
        ImGui::End();
    }
}

void MapViewer::maybeShowScoreboard() {
    if (showScoreboard) {
        scoreboard.draw("Scoreboard", &showScoreboard);
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

static bool excludeSetScratchArea[10000];
void MapViewer::maybeShowMatExclude()
{
    if (showMatExclude) {
        ImGui::Begin("Exclude Materials from World Mesh", &showMatExclude);
        std::vector<std::string> names = MAGNUMMATERIALMGR.getMaterialNames();
        int i = 0;
        for (auto name: names) {
            // Yes this is horrifyingly ugly
            ImGui::Checkbox(name.c_str(), (bool *)&excludeSetScratchArea[i]);
            i++;
        }
        if (ImGui::Button("Recompile with excludes")) {
            std::set<std::string> matset;
            for (int i = 0; i < names.size(); ++i) {
                if (excludeSetScratchArea[i]) matset.insert(names[i]);
            }
            worldRenderer.setExcludeSet(matset);
            worldRenderer.destroyWorldObject();
            worldRenderer.createWorldObject(&worldSceneBuilder);
            worldRenderer.getWorldObject()->setParent(&_manipulator);
        }
        if (ImGui::Button("Clear excludes")) {
            for (auto& x: excludeSetScratchArea) {
                x = 0;
            }
            worldRenderer.clearExcludeSet();
            worldRenderer.destroyWorldObject();
            worldRenderer.createWorldObject(&worldSceneBuilder);
            worldRenderer.getWorldObject()->setParent(&_manipulator);
        }
        ImGui::End();
    }
}

void MapViewer::onConsoleText(const char* msg) {
}

void MapViewer::drawEvent() {
    //_profiler.beginFrame();
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    _imgui.newFrame();
    /* Enable text input, if needed */
    if(ImGui::GetIO().WantTextInput && !isTextInputActive())
        startTextInput();
    else if(!ImGui::GetIO().WantTextInput && isTextInputActive())
        stopTextInput();

    showMainMenuBar();
    maybeShowConsole();
    maybeShowProfiler();
    maybeShowScoreboard();
    maybeShowTMBrowser();
    maybeShowMATBrowser();
    maybeShowMATViewer();
    maybeShowObsBrowser();
    maybeShowGLInfo();
    maybeShowMatExclude();

    /* Update application cursor */
    _imgui.updateApplicationCursor(*this);

    /* Reset state. Only needed if you want to draw something else with
       different state after. */
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    if (showGrid)
        if (auto* dg = worldRenderer.getDebugDrawableGroup())
            _camera->draw(*dg);
    if (auto* dg = worldRenderer.getDrawableGroup())
        _camera->draw(*dg);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    if (auto* dg = worldRenderer.getTransDrawableGroup())
        _camera->draw(*dg);

        /* Set appropriate states. If you only draw ImGui, it is sufficient to
       just enable blending and scissor test in the constructor. */
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

    _imgui.drawFrame();

    

    swapBuffers();
    //_profiler.endFrame();
    //_profiler.printStatistics(10);
}

/* This whole class is an extremely dirty hack just to get a basic client working.
   It should be replaced with something that isn't insane. */
class WorldDownLoader : cURLManager
{
public:
    WorldDownLoader(MapViewer *app);
    void start(char * hexDigest);
private:
    void askToBZFS();
    static int curlProgressFunc(void* UNUSED(clientp),
                     double dltotal, double dlnow,
                     double UNUSED(ultotal), double UNUSED(ulnow));
    virtual void finalization(char *data, unsigned int length, bool good);
    static MapViewer *_app;
};

MapViewer *WorldDownLoader::_app;

WorldDownLoader::WorldDownLoader(MapViewer *app) {
    _app = app;
}

void WorldDownLoader::start(char * hexDigest)
{
    if (_app->isCached(hexDigest))
        _app->loadCachedWorld();
    else if (_app->worldUrl.size())
    {
        //HUDDialogStack::get()->setFailedMessage
        _app->console.addLog(("Loading world from " + _app->worldUrl).c_str());
        //std::cout << "Loading world from " << _app->worldUrl;
        setProgressFunction(curlProgressFunc, _app->worldUrl.c_str());
        setURL(_app->worldUrl);
        addHandle();
        _app->worldUrl = ""; // clear the state
    }
    else
        askToBZFS();
}

int WorldDownLoader::curlProgressFunc(void* UNUSED(clientp),
                     double dltotal, double dlnow,
                     double UNUSED(ultotal), double UNUSED(ulnow))
{
    // update the status
    double percentage = 0.0;
    if ((int)dltotal > 0)
        percentage = 100.0 * dlnow / dltotal;
    char buffer[128];
    sprintf (buffer, "%2.1f%% (%i/%i)", percentage, (int)dlnow, (int)dltotal);
    _app->console.addLog(buffer);

    return 0;
}

void WorldDownLoader::finalization(char *data, unsigned int length, bool good)
{
    if (good)
    {
        _app->worldDatabase = data;
        theData       = NULL;
        MD5 md5;
        md5.update((unsigned char *)_app->worldDatabase, length);
        md5.finalize();
        std::string digest = md5.hexdigest();
        if (digest != _app->md5Digest)
        {
            //HUDDialogStack::get()->setFailedMessage("Download from URL failed");
            //std::cout << "Download from URL failed" << std::endl;
            _app->console.addLog("Download from URL failed");
            askToBZFS();
        }
        else
        {
            std::ostream* cache =
                FILEMGR.createDataOutStream(_app->worldCachePath, true, true);
            if (cache != NULL)
            {
                cache->write(_app->worldDatabase, length);
                delete cache;
                _app->loadCachedWorld();
            }
            else
            {
                //HUDDialogStack::get()->setFailedMessage("Problem writing cache");
                //std::cout << "Problem writing cache" << std::endl;
                _app->console.addLog("Problem writing cache");
                askToBZFS();
            }
        }
    }
    else
        askToBZFS();
}

void WorldDownLoader::askToBZFS() {
    //HUDDialogStack::get()->setFailedMessage("Downloading World...");
    //std::cout << "Downloading world..." << std::endl;
    _app->console.addLog("Downloading World...");
    char message[MaxPacketLen];
    // ask for world
    nboPackUInt(message, 0);
    //_app->_serverLink->send(MsgGetWorld, sizeof(uint32_t), message);
    _app->worldPtr = 0;
    if (_app->cacheOut)
        delete _app->cacheOut;
    _app->cacheOut = FILEMGR.createDataOutStream(_app->worldCachePath, true, true);
}

void MapViewer::markOld(std::string &fileName) {
    #ifdef _WIN32
    FILETIME ft;
    HANDLE h = CreateFile(fileName.c_str(),
                          FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA, 0, NULL,
                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE)
    {
        SYSTEMTIME st;
        memset(&st, 0, sizeof(st));
        st.wYear = 1900;
        st.wMonth = 1;
        st.wDay = 1;
        SystemTimeToFileTime(&st, &ft);
        SetFileTime(h, &ft, &ft, &ft);
        GetLastError();
        CloseHandle(h);
    }
#else
    struct utimbuf times;
    times.actime = 0;
    times.modtime = 0;
    utime(fileName.c_str(), &times);
#endif
}

void MapViewer::loadCachedWorld() {
    // can't get a cache from nothing
    if (worldCachePath == std::string(""))
    {
        joiningGame = false;
        return;
    }

    // lookup the cached world
    std::istream *cachedWorld = FILEMGR.createDataInStream(worldCachePath, true);
    if (!cachedWorld)
    {
        //HUDDialogStack::get()->setFailedMessage("World cache files disappeared.  Join canceled");
        //std::cout << "World cache files disappeared.  Join canceled" << std::endl;
        console.addLog("World cache files disappeared.  Join canceled");
        //drawFrame(0.0f);
        remove(worldCachePath.c_str());
        joiningGame = false;
        return;
    }

    // status update
    //HUDDialogStack::get()->setFailedMessage("Loading world into memory...");
    //drawFrame(0.0f);
    //std::cout << "Loading world into memory..." << std::endl;
    console.addLog("Loading world into memory...");

    // get the world size
    cachedWorld->seekg(0, std::ios::end);
    std::streampos size = cachedWorld->tellg();
    unsigned long charSize = (unsigned long)std::streamoff(size);

    // load the cached world
    cachedWorld->seekg(0);
    char *localWorldDatabase = new char[charSize];
    if (!localWorldDatabase)
    {
        //HUDDialogStack::get()->setFailedMessage("Error loading cached world.  Join canceled");
        //drawFrame(0.0f);
        //std::cout << "Error loading cached world." << std::endl;
        console.addLog("Error loading cached world.");
        remove(worldCachePath.c_str());
        joiningGame = false;
        return;
    }
    cachedWorld->read(localWorldDatabase, charSize);
    delete cachedWorld;

    // verify
    //HUDDialogStack::get()->setFailedMessage("Verifying world integrity...");
    //drawFrame(0.0f);
    //std::cout << "Verifying world" << std::endl;
    console.addLog("Verifying world integrity...");
    MD5 md5;
    md5.update((unsigned char *)localWorldDatabase, charSize);
    md5.finalize();
    std::string digest = md5.hexdigest();
    if (digest != md5Digest)
    {
        if (worldBuilder)
            delete worldBuilder;
        worldBuilder = NULL;
        delete[] localWorldDatabase;
        //HUDDialogStack::get()->setFailedMessage("Error on md5. Removing offending file.");
        //std::cout << "md5 error" << std::endl;
        console.addLog("Error on md5. Removing offending file.");
        remove(worldCachePath.c_str());
        joiningGame = false;
        return;
    }

    // make world
    //HUDDialogStack::get()->setFailedMessage("Preparing world...");
    //drawFrame(0.0f);
    //std::cout << "Preparing world..." << std::endl;
    console.addLog("Preparing world...");
    if (world)
    {
        delete world;
        world = NULL;
    }
    if (!worldBuilder->unpack(localWorldDatabase))
    {
        // world didn't make for some reason
        if (worldBuilder)
            delete worldBuilder;
        worldBuilder = NULL;
        delete[] localWorldDatabase;
        //HUDDialogStack::get()->setFailedMessage("Error unpacking world database. Join canceled.");
        //std::cout << "Error unpacking world db." << std::endl;
        console.addLog("Error unpacking world database. Join canceled.");
        remove(worldCachePath.c_str());
        joiningGame = false;
        return;
    }
    delete[] localWorldDatabase;

    // return world
    world = worldBuilder->getWorld();
    if (worldBuilder)
        delete worldBuilder;
    worldBuilder = NULL;

    //HUDDialogStack::get()->setFailedMessage("Downloading files...");
    console.addLog("Downloading files...");

    const bool doDownloads =  BZDB.isTrue("doDownloads");
    const bool updateDownloads =  BZDB.isTrue("updateDownloads");
    Downloads::startDownloads(doDownloads, updateDownloads, false);
    downloadingInitialTexture  = true;
}

bool MapViewer::isCached(char *hexDigest) {
    std::istream *cachedWorld;
    bool cached    = false;
    worldCachePath = getCacheDirName();
    worldCachePath += hexDigest;
    worldCachePath += ".bwc";
    if ((cachedWorld = FILEMGR.createDataInStream(worldCachePath, true)))
    {
        cached = true;
        delete cachedWorld;
    }
    return cached;
}

void MapViewer::tickEvent() {
    redraw();
}

void MapViewer::enteringServer(const void *buf) {

    entered = true;
}

void MapViewer::setTankFlags()
{
}

void MapViewer::updateFlag(FlagType *flag) {
}

void MapViewer::viewportEvent(ViewportEvent& e) {
    GL::defaultFramebuffer.setViewport({{}, e.framebufferSize()});
    _imgui.relayout(Vector2{e.windowSize()}/e.dpiScaling(),
        e.windowSize(), e.framebufferSize());
    _camera->setViewport(e.windowSize());
}

void MapViewer::keyPressEvent(KeyEvent& event) {
    if(_imgui.handleKeyPressEvent(event)) return;
}

void MapViewer::keyReleaseEvent(KeyEvent& event) {
    if(_imgui.handleKeyReleaseEvent(event)) return;
}

void MapViewer::mousePressEvent(MouseEvent& e) {
    if(_imgui.handleMousePressEvent(e)) return;
    if (e.button() == MouseEvent::Button::Left)
        _previousPosition = positionOnSphere(e.position());
}

void MapViewer::mouseReleaseEvent(MouseEvent& e) {
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
    if (!e.offset().y()) return;
    const Float distance = _cameraObject.transformation().translation().z();
    _cameraObject.translate(Vector3::zAxis(distance*(1.0f - (e.offset().y() > 0 ? 1/0.85f : 0.85f))));
    redraw();
}

void MapViewer::mouseMoveEvent(MouseMoveEvent &e) {
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
    if(_imgui.handleTextInputEvent(e)) return;
}

void MapViewer::exitEvent(ExitEvent& e) {
    isQuit = true;
}

Vector3 MapViewer::positionOnSphere(const Vector2i& position) const {
    const Vector2 positionNormalized = Vector2{position}/Vector2{_camera->viewport()} - Vector2{0.5f};
    const Float length = positionNormalized.length();
    const Vector3 result(length > 1.0f ? Vector3(positionNormalized, 0.0f) : Vector3(positionNormalized, 1.0f - length));
    return (result * Vector3::yScale(-1.0f)).normalized();
}

int MapViewer::main() {
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

    BZDBCache::init();
    BZDBLOCAL.init();

    ::Flags::init();

    time_t timeNow;
    time(&timeNow);
    userTime = *localtime(&timeNow);

    unsigned int i;

    loadBZDBDefaults();

    Team::updateShotColors();

    // TM test
    MagnumTextureManager &tm = MagnumTextureManager::instance();
    tm.getTexture("boxwall");
    tm.getTexture("moon");
    tm.getTexture("mountain1");
    tm.getTexture("tetrawall");

    MAGNUMMATERIALMGR.loadDefaultMaterials();

    worldRenderer.createWorldObject(&worldSceneBuilder);
    worldRenderer.getWorldObject()->setParent(&_manipulator);

    startPlaying();

    tm.clear();
    exit(0);
    return 0;
}

void MapViewer::joinInternetGame2()
{
    justJoined = true;

    World::setWorld(world);

    // prep teams
    teams = world->getTeams();


    worldRenderer.destroyWorldObject();

    world->makeMeshDrawMgrs();

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
    
    world->makeLinkMaterial();
    
    worldRenderer.createWorldObject(&worldSceneBuilder);
    worldRenderer.getWorldObject()->setParent(&_manipulator);

    joiningGame = false;
}

const void *MapViewer::handleMsgSetVars(const void *msg) {
    
}

void MapViewer::startupErrorCallback(const char* msg)
{
    Warning{} << msg;
}

void MapViewer::startPlaying()
{
    lastObserverUpdateTime = TimeKeeper::getTick().getSeconds();

    setErrorCallback(startupErrorCallback);

    World::init();

    // prepare dialogs / main menu

    setErrorCallback(startupErrorCallback);

    TimeKeeper::setTick();

    worldDownLoader = new WorldDownLoader(this);

    playingLoop();

    delete worldDownLoader;

    leaveGame();
    World::done();
}

void MapViewer::playingLoop()
{
        // main loop
    //while (!CommandsStandard::isQuit())
    while (!isQuit)
    {

        BZDBCache::update();

        // set this step game time
        GameTime::setStepTime();

        // get delta time
        TimeKeeper prevTime = TimeKeeper::getTick();
        TimeKeeper::setTick();
        const float dt = float(TimeKeeper::getTick() - prevTime);

        // TODO: Update sky every few seconds
        if (world)
            world->updateWind(dt);

        // TODO: Make this work!
        if (world)
            world->updateAnimations(dt);

        // draw frame
        // update the dynamic colors
        DYNCOLORMGR.update();

        // update the texture matrices
        TEXMATRIXMGR.update();
        mainLoopIteration();

        // check if we are waiting for initial texture downloading
                // check if we are waiting for initial texture downloading
        if (Downloads::requestFinalized())
        {
            // downloading is terminated. go!
            Downloads::finalizeDownloads();
            if (downloadingInitialTexture)
            {
                joinInternetGame2();
                downloadingInitialTexture = false;
            }
        }
    }
}

void MapViewer::updateFlags(float dt) {
}

bool MapViewer::processWorldChunk(const void *buf, uint16_t len, int bytesLeft) {
    int totalSize = worldPtr + len + bytesLeft;
    int doneSize  = worldPtr + len;
    if (cacheOut)
        cacheOut->write((const char *)buf, len);
    /*HUDDialogStack::get()->setFailedMessage
    (TextUtils::format
     ("Downloading World (%2d%% complete/%d kb remaining)...",
      (100 * doneSize / totalSize), bytesLeft / 1024).c_str());*/
    //std::cout << "Downloading World..." << std::endl;
    console.addLog("Downloading World (%2d%% complete/%d kb remaining)...",
      (100 * doneSize / totalSize), bytesLeft / 1024);
    return bytesLeft == 0;
}

void MapViewer::checkEnvironment() {
}

void MapViewer::doMotion() {
        // Implement based on playing.cxx
}

void MapViewer::joinInternetGame()
{
    joiningGame = true;
    GameTime::reset();
}

void MapViewer::leaveGame() {
    entered = false;
    joiningGame = false;

    // Clear the world scene
    worldRenderer.destroyWorldObject();

    // Reset the scene builder
    worldSceneBuilder.reset();

    // reset the daylight time
    const bool syncTime = (BZDB.eval(StateDatabase::BZDB_SYNCTIME) >= 0.0f);
    const bool fixedTime = BZDB.isSet("fixedTime");

    // flush downloaded textures (before the BzMaterials are nuked)
    Downloads::removeTextures();

    // delete world
    World::setWorld(NULL);
    delete world;
    world = NULL;
    teams = NULL;

    serverError = false;
    serverDied = false;

    // reset the BZDB variables
    BZDB.iterate(MapViewer::resetServerVar, NULL);

    ::Flags::clearCustomFlags();
}

void MapViewer::resetServerVar(const std::string& name, void*)
{
    // reset server-side variables
    if (BZDB.getPermission(name) == StateDatabase::Locked)
    {
        const std::string defval = BZDB.getDefault(name);
        BZDB.set(name, defval);
    }
}

int main(int argc, char** argv)
{
    MapViewer app({argc, argv});
    return app.main();
}

//MAGNUM_APPLICATION_MAIN(MapViewer)