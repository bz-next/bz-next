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

#include "BZChatConsole.h"

#include <ctime>
#include <cassert>
#include <imgui.h>
#include <sstream>
#include <cstring>
#include <functional>
#include "Magnum/Types.h"
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

// defaults for bzdb
#include "defaultBZDB.h"

bool            echoToConsole = false;
bool            echoAnsi = false;
int         debugLevel = 0;
std::string     alternateConfig;
struct tm       userTime;
const char*     argv0;

static const char*  blowedUpMessage[] =
{
    NULL,
    "Got shot by ",
    "Got flattened by ",
    "Team flag was captured by ",
    "Teammate hit with Genocide by ",
    "Tank Self Destructed",
    "Tank Rusted"
};

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

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

class ColoredDrawable : public SceneGraph::Drawable3D {
    public:
        explicit ColoredDrawable(Object3D& object, Shaders::PhongGL& shader, GL::Mesh& mesh, const Color4& color, SceneGraph::DrawableGroup3D& group) :
            SceneGraph::Drawable3D{object, &group},
            _shader(shader),
            _mesh(mesh),
            _color(color)
        {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Shaders::PhongGL& _shader;
        GL::Mesh& _mesh;
        Color4 _color;
};

class InstancedColoredDrawable : public SceneGraph::Drawable3D {
    public:
        explicit InstancedColoredDrawable(Object3D& object, Shaders::PhongGL& shader, GL::Mesh& mesh, SceneGraph::DrawableGroup3D& group) :
            SceneGraph::Drawable3D{object, &group},
            _shader(shader),
            _mesh(mesh)
        {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Shaders::PhongGL& _shader;
        GL::Mesh& _mesh;
};

void InstancedColoredDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setLightPositions({{camera.cameraMatrix().transformPoint({0.0f, 0.0f, 1000.0f}), 0.0f}})
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .draw(_mesh);
}

class TexturedDrawable : public SceneGraph::Drawable3D {
    public:
        explicit TexturedDrawable(Object3D& object, Shaders::PhongGL& shader, GL::Mesh& mesh, GL::Texture2D& texture, SceneGraph::DrawableGroup3D& group) :
            SceneGraph::Drawable3D{object, &group},
            _shader(shader),
            _mesh(mesh),
            _texture(texture)
        {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Shaders::PhongGL& _shader;
        GL::Mesh& _mesh;
        GL::Texture2D &_texture;
};

void ColoredDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setDiffuseColor(_color)
        .setLightPositions({{camera.cameraMatrix().transformPoint({0.0f, 0.0f, 1000.0f}), 0.0f}})
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .draw(_mesh);
}

void TexturedDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setLightPositions({{camera.cameraMatrix().transformPoint({0.0f, 0.0f, 1000.0f}), 0.0f}})
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .bindDiffuseTexture(_texture)
        .draw(_mesh);
}

class WorldPrimitiveGenerator {
    public:
        WorldPrimitiveGenerator() = delete;

        static Trade::MeshData pyrSolid();
        static Trade::MeshData wall();
    private:


};

Trade::MeshData WorldPrimitiveGenerator::pyrSolid() {
    // Create pyr mesh
    Vector3 pyrVerts[5] =
    {
        {-1.0f, -1.0f, 0.0f},
        {+1.0f, -1.0f, 0.0f},
        {+1.0f, +1.0f, 0.0f},
        {-1.0f, +1.0f, 0.0f},
        { 0.0f,  0.0f, 1.0f}
    };

    Magnum::UnsignedShort pyrIndices[] =
    {
        // base
        2, 1, 0,
        0, 3, 2,
        0, 4, 3,
        3, 4, 2,
        2, 4, 1,
        4, 0, 1
    };

    struct Vertex {
        Vector3 position;
        Vector3 normal;
    };

    Containers::Array<Vector3> positions = MeshTools::duplicate<typeof pyrIndices[0], Vector3>(pyrIndices, pyrVerts);
    Containers::Array<Vector3> normals = MeshTools::generateFlatNormals(positions);

    Containers::Array<char> vertexData{positions.size() * sizeof(Vertex)};
    vertexData = MeshTools::interleave(positions, normals);

    Containers::StridedArrayView1D<const Vertex> vertices = Containers::arrayCast<const Vertex>(vertexData);

    Trade::MeshAttributeData pyrAttributes[] {
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, vertices.slice(&Vertex::position)},
        Trade::MeshAttributeData{Trade::MeshAttribute::Normal, vertices.slice(&Vertex::normal)}
    };
    
    return Trade::MeshData{MeshPrimitive::Triangles, std::move(vertexData),
        {
            Trade::MeshAttributeData{Trade::MeshAttribute::Position, vertices.slice(&Vertex::position)},
            Trade::MeshAttributeData{Trade::MeshAttribute::Normal, vertices.slice(&Vertex::normal)}
        }, (Magnum::UnsignedInt)positions.size()};
}

Trade::MeshData WorldPrimitiveGenerator::wall() {
    // Create pyr mesh
    Vector3 pyrVerts[5] =
    {
        {-1.0f, -1.0f, 0.0f},
        {+1.0f, -1.0f, 0.0f},
        {+1.0f, +1.0f, 0.0f},
        {-1.0f, +1.0f, 0.0f},
        { 0.0f,  0.0f, 1.0f}
    };

    Magnum::UnsignedShort pyrIndices[] =
    {
        // base
        2, 1, 0,
        0, 3, 2,
        0, 4, 3,
        3, 4, 2,
        2, 4, 1,
        4, 0, 1
    };

    struct Vertex {
        Vector3 position;
        Vector3 normal;
    };

    Containers::Array<Vector3> positions = MeshTools::duplicate<typeof pyrIndices[0], Vector3>(pyrIndices, pyrVerts);
    Containers::Array<Vector3> normals = MeshTools::generateFlatNormals(positions);

    Containers::Array<char> vertexData{positions.size() * sizeof(Vertex)};
    vertexData = MeshTools::interleave(positions, normals);

    Containers::StridedArrayView1D<const Vertex> vertices = Containers::arrayCast<const Vertex>(vertexData);

    Trade::MeshAttributeData pyrAttributes[] {
        Trade::MeshAttributeData{Trade::MeshAttribute::Position, vertices.slice(&Vertex::position)},
        Trade::MeshAttributeData{Trade::MeshAttribute::Normal, vertices.slice(&Vertex::normal)}
    };
    
    return Trade::MeshData{MeshPrimitive::Triangles, std::move(vertexData),
        {
            Trade::MeshAttributeData{Trade::MeshAttribute::Position, vertices.slice(&Vertex::position)},
            Trade::MeshAttributeData{Trade::MeshAttribute::Normal, vertices.slice(&Vertex::normal)}
        }, (Magnum::UnsignedInt)positions.size()};
}

class WorldRenderer {
    public:
        WorldRenderer();
        ~WorldRenderer();
        // Assumes that world is already loaded and OBSTACLEMGR is ready to go
        void createWorldObject();

        SceneGraph::DrawableGroup3D *getDrawableGroup();
        Object3D *getWorldObject();

        void destroyWorldObject();
    private:
        struct InstanceData {
            Matrix4 transformationMatrix;
            Matrix3x3 normalMatrix;
            Color3 color;
        };
        std::map<std::string, std::list<GL::Mesh>> worldMeshes;
        Object3D *worldParent;
        SceneGraph::DrawableGroup3D *worldDrawables;
        Shaders::PhongGL coloredShader;
        Shaders::PhongGL coloredShaderInstanced{Shaders::PhongGL::Configuration{}
            .setFlags(Shaders::PhongGL::Flag::InstancedTransformation|
                    Shaders::PhongGL::Flag::VertexColor)};
};

SceneGraph::DrawableGroup3D *WorldRenderer::getDrawableGroup()
{
    return worldDrawables;
}

Object3D *WorldRenderer::getWorldObject()
{
    return worldParent;
}

WorldRenderer::WorldRenderer() {
    //worldMeshes["cube"].push_back(MeshTools::compile(Primitives::cubeSolid()));
    //worldMeshes["pyr"].push_back(MeshTools::compile(WorldPrimitiveGenerator::pyrSolid()));
    worldDrawables = NULL;
    worldParent = new Object3D{};
}

WorldRenderer::~WorldRenderer() {
    //worldMeshes["cube"].push_back(MeshTools::compile(Primitives::cubeSolid()));
    //worldMeshes["pyr"].push_back(MeshTools::compile(WorldPrimitiveGenerator::pyrSolid()));
    if (worldDrawables) delete worldDrawables;
    delete worldParent;
}

void WorldRenderer::createWorldObject() {
    worldDrawables = new SceneGraph::DrawableGroup3D{};
    // Perhaps do culling here in the future if necessary?
    // Could construct instance buffer per-frame.
    auto buildInstanceVectorFromList = [](const ObstacleList &l, Color3 color) {
        std::vector<InstanceData> instances;
        for (int i = 0; i < l.size(); ++i) {
            Object3D obj;
            InstanceData thisInstance;
            
            const float *pos = l[i]->getPosition();
            const float *sz = l[i]->getSize();

            obj.rotateZ(Rad(l[i]->getRotation()));
            obj.translate(Vector3{pos[0], pos[1], pos[2]});
            obj.scaleLocal(Vector3{sz[0], sz[1], sz[2]});

            thisInstance.transformationMatrix = obj.transformationMatrix();
            thisInstance.normalMatrix = obj.transformationMatrix().normalMatrix();

            thisInstance.color = color;
            instances.push_back(thisInstance);
        }
        return instances;
    };
    // Create world box instances (using instanced rendering for performance)
    {
        Object3D *worldCubes = new Object3D;

        worldMeshes["instances"].push_back(MeshTools::compile(Primitives::cubeSolid()));
        GL::Mesh *worldBoxMesh = &worldMeshes["instances"].back();

        std::vector<InstanceData> instances = buildInstanceVectorFromList(OBSTACLEMGR.getBoxes(), 0xCC5511_rgbf);

        worldCubes->setParent(worldParent);
        worldBoxMesh->addVertexBufferInstanced(GL::Buffer{instances}, 1, 0,
            Shaders::PhongGL::TransformationMatrix{},
            Shaders::PhongGL::NormalMatrix{},
            Shaders::PhongGL::Color3{});
        worldBoxMesh->setInstanceCount(instances.size());
        new InstancedColoredDrawable(*worldCubes, coloredShaderInstanced, *worldBoxMesh, *worldDrawables);
    }
    // Create world pyramids
    {
        const ObstacleList& l = OBSTACLEMGR.getPyrs();
        for (int i = 0; i < l.size(); ++i) {
            Object3D *worldPyrs = new Object3D;

            worldMeshes["instances"].push_back(MeshTools::compile(WorldPrimitiveGenerator::pyrSolid()));
            GL::Mesh *worldPyrMesh = &worldMeshes["instances"].back();

            std::vector<InstanceData> instances = buildInstanceVectorFromList(OBSTACLEMGR.getPyrs(), 0x1155CC_rgbf);

            worldPyrs->setParent(worldParent);
            worldPyrMesh->addVertexBufferInstanced(GL::Buffer{instances}, 1, 0,
                Shaders::PhongGL::TransformationMatrix{},
                Shaders::PhongGL::NormalMatrix{},
                Shaders::PhongGL::Color3{});
            worldPyrMesh->setInstanceCount(instances.size());
            new InstancedColoredDrawable(*worldPyrs, coloredShaderInstanced, *worldPyrMesh, *worldDrawables);
        }
    }
}

void WorldRenderer::destroyWorldObject() {
    delete worldDrawables;
    delete worldParent;
    worldParent = new Object3D{};
    worldDrawables = NULL;
}


class BZFlagNew: public Platform::Sdl2Application {
    public:
        explicit BZFlagNew(const Arguments& arguments);
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

        void maybeShowConsole();
        bool showConsole = true;

        void maybeShowFPS();
        bool showFPS = false;

        void maybeShowConnect();
        bool showConnect = true;
        
        Vector3 positionOnSphere(const Vector2i& position) const;

        friend class WorldDownLoader;
        void startPlaying();
        void playingLoop();

        void onConsoleText(const char* txt);

        static void startupErrorCallback(const char* msg);

        void initAres();
        void killAres();

        void joinInternetGame();
        void joinInternetGame2();
        void sendFlagNegotiation();
        void leaveGame();

        void addMessage(const Player *_player, const std::string& msg, int mode = 3,
                           bool highlight = false,
                           const char* oldColor = NULL);

        void setTankFlags();
        void updateFlag(FlagType *flag);

        void doMotion();

        void enteringServer(const void *buf);

        bool isKillable(const Player* target);
        void checkEnvironment();

        void updateFlags(float dt);

        void doMessages();

        bool isCached(char *hexDigest);
        void loadCachedWorld();

        Player* addPlayer(PlayerId id, const void* msg, int showMessage);
        bool removePlayer(PlayerId id);
        ServerLink* lookupServer(const Player *_player);
        void handleFlagDropped(Player* tank);

        bool gotBlowedUp(BaseLocalPlayer* tank, BlowedUpReason reason, PlayerId killer, const ShotPath* hit = NULL, int phydrv = -1);

        const void *handleMsgSetVars(const void *msg);
        void handlePlayerMessage(uint16_t, uint16_t, const void*);

        void markOld(std::string &fileName);

        void handleFlagTransferred(Player *fromTank, Player *toTank, int flagIndex);

        void handleServerMessage(bool human, uint16_t code, uint16_t len, const void* msg);
        bool processWorldChunk(const void *buf, uint16_t len, int bytesLeft);

        WorldDownLoader *worldDownLoader;
        double lastObserverUpdateTime = -1;
        StartupInfo startupInfo;
        MessageOfTheDay *motd;
        bool joinRequested = false;
        bool waitingDNS = false;
        Address serverNetworkAddress;
        bool justJoined = false;
        ServerLink *_serverLink = NULL;
        bool serverDied = false;
        bool serverError = false;
        bool joiningGame = false;
        bool entered = false;
        LocalPlayer *myTank = NULL;
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
        ShotStats *shotStats = NULL;
        bool wasRabbit = false;
        char *worldDatabase = NULL;
        std::ostream *cacheOut = NULL;

        DebugTools::FrameProfilerGL _profiler{DebugTools::FrameProfilerGL::Value::FrameTime|
    DebugTools::FrameProfilerGL::Value::GpuDuration, 50};

        std::vector<PlayingCallbackItem> playingCallbacks;

        AccessList ServerAccessList;

        AresHandler *ares = NULL;

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
        //std::vector<ColoredDrawable *> tankDrawables;
        ImGuiIntegration::Context _imgui{NoCreate};

        BZChatConsole console;

        bool isQuit = false;
};

BZFlagNew::BZFlagNew(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("BZFlag Magnum Test")
        .setWindowFlags(Configuration::WindowFlag::Resizable)},
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
        .translate(Vector3::zAxis(5.0f));
    
    (*(_camera = new SceneGraph::Camera3D{_cameraObject}))
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.01f, 10000.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());
    
    _manipulator.setParent(&_scene);


    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

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

void BZFlagNew::showMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::MenuItem("Connect", NULL, &showConnect)) {
        }
        if (ImGui::BeginMenu("View")) {
            showMenuView();
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void BZFlagNew::showMenuView() {
    if (ImGui::MenuItem("Console", NULL, &showConsole)) {}
    if (ImGui::MenuItem("FPS", NULL, &showFPS)) {}
}

void BZFlagNew::maybeShowConsole() {
    if (showConsole)
        console.draw("Console", NULL);
}

void BZFlagNew::maybeShowFPS() {
    if (showFPS) {
        ImGui::Begin("FPS", NULL);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
            1000.0/Double(ImGui::GetIO().Framerate), Double(ImGui::GetIO().Framerate));
        ImGui::End();
    }
}

void BZFlagNew::maybeShowConnect() {

    static char callsign[128];
    static char password[128];
    static char hostname[128];
    static char port[128];

    if (!showConnect) return;

    ImGui::Begin("Connect", &showConnect, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::InputText("Callsign", callsign, IM_ARRAYSIZE(callsign));
    ImGui::InputText("Password", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password);
    ImGui::InputText("Hostname", hostname, IM_ARRAYSIZE(hostname));
    ImGui::InputText("Port", port, IM_ARRAYSIZE(port));
    if (ImGui::Button("Connect")) {
        tryConnect(callsign, password, hostname, port);
    }
    ImGui::End();
}

void BZFlagNew::onConsoleText(const char* msg) {
    // local commands:
    // /set lists all BZDB variables
    /*if (msg == "/set")
    {
        if (ui != NULL)
            BZDB.iterate(listSetVars, this);
        return;
    }*/

    if (entered) {
        // Flawfinder: ignore
        char buffer[MessageLen];
        // Flawfinder: ignore
        char buffer2[1 + MessageLen];
        void* buf = buffer2;

        buf = nboPackUByte(buf, AllPlayers);
        // Flawfinder: ignore
        strncpy(buffer, msg, MessageLen - 1);
        buffer[MessageLen - 1] = '\0';
        nboPackString(buffer2 + 1, buffer, MessageLen);
        _serverLink->send(MsgMessage, sizeof(buffer2), buffer2);
    }
}

void BZFlagNew::drawEvent() {
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
    maybeShowFPS();
    maybeShowConnect();

    /* Update application cursor */
    _imgui.updateApplicationCursor(*this);

    /* Reset state. Only needed if you want to draw something else with
       different state after. */
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    if (auto* dg = worldRenderer.getDrawableGroup())
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

class WorldDownLoader : cURLManager
{
public:
    WorldDownLoader(BZFlagNew *app);
    void start(char * hexDigest);
private:
    void askToBZFS();
    static int curlProgressFunc(void* UNUSED(clientp),
                     double dltotal, double dlnow,
                     double UNUSED(ultotal), double UNUSED(ulnow));
    virtual void finalization(char *data, unsigned int length, bool good);
    static BZFlagNew *_app;
};

BZFlagNew *WorldDownLoader::_app;

WorldDownLoader::WorldDownLoader(BZFlagNew *app) {
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
    _app->_serverLink->send(MsgGetWorld, sizeof(uint32_t), message);
    _app->worldPtr = 0;
    if (_app->cacheOut)
        delete _app->cacheOut;
    _app->cacheOut = FILEMGR.createDataOutStream(_app->worldCachePath, true, true);
}

void BZFlagNew::markOld(std::string &fileName) {
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

void BZFlagNew::loadCachedWorld() {
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

void BZFlagNew::addMessage(const Player *_player, const std::string& msg, int mode, bool highlight, const char* oldColor) {
    std::string fullMessage;

    if (BZDB.isTrue("colorful"))
    {
        if (_player)
        {
            if (highlight)
            {
                if (BZDB.get("killerhighlight") == "1")
                    fullMessage += ColorStrings[PulsatingColor];
                else if (BZDB.get("killerhighlight") == "2")
                    fullMessage += ColorStrings[UnderlineColor];
            }
            const PlayerId pid = _player->getId();
            if (pid < 200)
            {
                TeamColor color = _player->getTeam();
                fullMessage += Team::getAnsiCode(color);
            }
            else if (pid == ServerPlayer)
                fullMessage += ColorStrings[YellowColor];
            else
            {
                fullMessage += ColorStrings[CyanColor]; //replay observers
            }
            fullMessage += _player->getCallSign();

            if (highlight)
                fullMessage += ColorStrings[ResetColor];
#ifdef BWSUPPORT
            fullMessage += " (";
            fullMessage += Team::getName(_player->getTeam());
            fullMessage += ")";
#endif
            fullMessage += ColorStrings[DefaultColor] + ": ";
        }
        fullMessage += msg;
    }
    else
    {
        if (oldColor != NULL)
            fullMessage = oldColor;

        if (_player)
        {
            fullMessage += _player->getCallSign();

#ifdef BWSUPPORT
            fullMessage += " (";
            fullMessage += Team::getName(_player->getTeam());
            fullMessage += ")";
#endif
            fullMessage += ": ";
        }
        fullMessage += stripAnsiCodes(msg);
    }
    //controlPanel->addMessage(fullMessage, mode);
    //std::cout << fullMessage << std::endl;
    console.addLog(fullMessage.c_str());
}

bool BZFlagNew::isCached(char *hexDigest) {
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

void BZFlagNew::initAres()
{
    ares = new AresHandler(0);
}

void BZFlagNew::killAres()
{
    delete ares;
    ares = NULL;
}

void BZFlagNew::tickEvent() {
    /*static bool haveWeDoneIt = false;
    if (world != NULL && entered && !haveWeDoneIt) {

        worldRenderer.createWorldObject();
        haveWeDoneIt = true;
    }*/
    //static bool haveWeDoneIt2 = false;
    /*if (entered && !haveWeDoneIt2) {
        //tankObjs.resize(curMaxPlayers);
        for (int i = 0; i < curMaxPlayers; ++i) {
            Object3D *o = new Object3D{};
            o->setParent(&_manipulator);
            new ColoredDrawable(*o, _coloredShader, *_meshes[0], 0xEE1111_rgbf, _drawables);
            tankObjs.push_back(o);
        }
        haveWeDoneIt2 = true;
    }*/

    /*if (entered) {
        for (int i = 0; i < curMaxPlayers; ++i) {
            if (remotePlayers[i]) {
                const float *pos = remotePlayers[i]->getPosition();
                //std::cout << i << " " << tankObjs.size() << " HERE" << std::endl;
                tankObjs[i]->resetTransformation();
                tankObjs[i]->translate(Vector3{{0.0f, 0.0f, 1.0f}});
                tankObjs[i]->scale(Vector3{10.0, 10.0, 10.0});
                tankObjs[i]->translate(Vector3{pos[0], pos[1], pos[2]});
                
                //Warning{} << Vector3{pos[0], pos[1], pos[2]};
            }
        }
    }*/

    redraw();



}

void BZFlagNew::enteringServer(const void *buf) {
        // the server sends back the team the player was joined to
    const void *tmpbuf = buf;
    uint16_t team, type, wins, losses, tks;
    char callsign[CallSignLen];
    char motto[MottoLen];
    tmpbuf = nboUnpackUShort(tmpbuf, type);
    tmpbuf = nboUnpackUShort(tmpbuf, team);
    tmpbuf = nboUnpackUShort(tmpbuf, wins);           // not used
    tmpbuf = nboUnpackUShort(tmpbuf, losses);         // not used
    tmpbuf = nboUnpackUShort(tmpbuf, tks);            // not used
    tmpbuf = nboUnpackString(tmpbuf, callsign, CallSignLen);  // not used
    tmpbuf = nboUnpackString(tmpbuf, motto, MottoLen);

    // if server assigns us a different team, display a message
    std::string teamMsg;
    if (myTank->getTeam() != AutomaticTeam)
    {
        teamMsg = TextUtils::format("%s team was unavailable, you were joined ",
                                    Team::getName(myTank->getTeam()));
        if ((TeamColor)team == ObserverTeam)
            teamMsg += "as an Observer";
        else
        {
            teamMsg += TextUtils::format("to the %s",
                                         Team::getName((TeamColor)team));
        }
    }
    else
    {
        if ((TeamColor)team == ObserverTeam)
            teamMsg = "You were joined as an observer";
        else
        {
            if (team != RogueTeam)
                teamMsg = TextUtils::format("You joined the %s",
                                            Team::getName((TeamColor)team));
            else
                teamMsg = TextUtils::format("You joined as a %s",
                                            Team::getName((TeamColor)team));
        }
    }
    if (myTank->getTeam() != (TeamColor)team)
    {
        myTank->setTeam((TeamColor)team);
        //hud->setAlert(1, teamMsg.c_str(), 8.0f,
        //              (TeamColor)team==ObserverTeam?true:false);
        addMessage(NULL, teamMsg.c_str(), 3, true);
        //std::cout << teamMsg << std::endl;
        console.addLog(teamMsg.c_str());
    }

    // observer colors are actually cyan, make them black
    const bool observer = (myTank->getTeam() == ObserverTeam);
    const GLfloat* borderColor;
    if (observer)
    {
        static const GLfloat black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        borderColor = black;
    }
    else
        borderColor = Team::getRadarColor(myTank->getTeam());
    //controlPanel->setControlColor(borderColor);
    //radar->setControlColor(borderColor);

    if ((myTank->getTeam() == ObserverTeam) || devDriving)
    {
        const std::string roamStr = BZDB.get("roamView");
        Roaming::RoamingView roamView = ROAM.parseView(roamStr);
        if (roamView <= Roaming::roamViewDisabled)
            roamView = Roaming::roamViewFP;
        ROAM.setMode(roamView);
            ROAM.resetCamera();
    }
    else
        ROAM.setMode(Roaming::roamViewDisabled);

    myTank->setMotto(motto);  // use motto provided by the server

    setTankFlags();

    // clear now invalid token
    startupInfo.token[0] = '\0';

    // resize background and adjust time (this is needed even if we
    // don't sync with the server)
    //sceneRenderer->getBackground()->resize();
    float syncTime = BZDB.eval(StateDatabase::BZDB_SYNCTIME);
    /*if (syncTime < 0.0f)
        updateDaylight(epochOffset, *sceneRenderer);
    else
    {
        epochOffset = (double)syncTime;
        updateDaylight(epochOffset, *sceneRenderer);
    }
    lastEpochOffset = epochOffset;*/

    // restore the sound
    if (savedVolume != -1)
    {
        //setSoundVolume(savedVolume);
        savedVolume = -1;
    }

    // initialize some other stuff
    updateFlag(::Flags::Null);
    //updateHighScores();
    //hud->setHeading(myTank->getAngle());
    //hud->setAltitude(myTank->getPosition()[2]);
    //hud->setTimeLeft((uint32_t)~0);
    fireButton = false;
    firstLife = true;

    BZDB.setBool("displayMainFlags", true);
    BZDB.setBool("displayRadarFlags", true);
    BZDB.setBool("displayRadar", true);
    BZDB.setBool("displayConsole", true);

    entered = true;
}

void BZFlagNew::setTankFlags()
{
    // scan through flags and, for flags on
    // tanks, tell the tank about its flag.
    const int maxFlags = world->getMaxFlags();
    for (int i = 0; i < maxFlags; i++)
    {
        const ::Flag& flag = world->getFlag(i);
        if (flag.status == FlagOnTank)
        {
            for (int j = 0; j < curMaxPlayers; j++)
            {
                if (remotePlayers[j] && remotePlayers[j]->getId() == flag.owner)
                {
                    remotePlayers[j]->setFlag(flag.type);
                    break;
                }
            }
        }
    }
}

void BZFlagNew::updateFlag(FlagType *flag) {
    if (flag == ::Flags::Null)
    {
        //hud->setColor(1.0f, 0.625f, 0.125f);
        //hud->setAlert(2, NULL, 0.0f);
    }
    else
    {
        const float* color = flag->getColor();
        //hud->setColor(color[0], color[1], color[2]);
        //hud->setAlert(2, flag->flagName.c_str(), 3.0f, flag->endurance == FlagSticky);
    }

    //if (BZDB.isTrue("displayFlagHelp"))
        //hud->setFlagHelp(flag, FlagHelpDuration);

    //TODO radar
    //if ((!radar && !myTank) || !World::getWorld())
    //    return;

    //radar->setJammed(flag == Flags::Jamming);
    //hud->setAltitudeTape(flag == Flags::Jumping || World::getWorld()->allowJumping());
}

Player* BZFlagNew::addPlayer(PlayerId id, const void* msg, int showMessage) {
    uint16_t team, type, wins, losses, tks;
    char callsign[CallSignLen];
    char motto[MottoLen];
    msg = nboUnpackUShort (msg, type);
    msg = nboUnpackUShort (msg, team);
    msg = nboUnpackUShort (msg, wins);
    msg = nboUnpackUShort (msg, losses);
    msg = nboUnpackUShort (msg, tks);
    msg = nboUnpackString (msg, callsign, CallSignLen);
    msg = nboUnpackString (msg, motto, MottoLen);

    // Strip any ANSI color codes
    strncpy (callsign, stripAnsiCodes (std::string (callsign)).c_str (), 31);

    // id is slot, check if it's empty
    const int i = id;

    // sanity check
    if (i < 0)
    {
        printError (TextUtils::format ("Invalid player identification (%d)", i));
        std::
        cerr <<
             "WARNING: invalid player identification when adding player with id "
             << i << std::endl;
        return NULL;
    }

    if (remotePlayers[i])
    {
        // we're not in synch with server -> help! not a good sign, but not fatal.
        printError ("Server error when adding player, player already added");
        std::cerr << "WARNING: player already exists at location with id "
                  << i << std::endl;
        return NULL;
    }

    if (i >= curMaxPlayers)
    {
        curMaxPlayers = i + 1;
        World::getWorld ()->setCurMaxPlayers (curMaxPlayers);
    }

    // add player
    if (PlayerType (type) == TankPlayer || PlayerType (type) == ComputerPlayer)
    {
        remotePlayers[i] = new RemotePlayer (id, TeamColor (team), callsign, motto,
                                             PlayerType (type));
        remotePlayers[i]->changeScore (short (wins), short (losses), short (tks));
    }

    // show the message if we don't have the playerlist
    // permission.  if * we do, MsgAdminInfo should arrive
    // with more info.
    if (showMessage && !myTank->hasPlayerList ())
    {
        std::string message ("joining as ");
        if (team == ObserverTeam)
            message += "an observer";
        else
        {
            switch (PlayerType (type))
            {
            case TankPlayer:
                message += "a tank";
                break;
            case ComputerPlayer:
                message += "a robot tank";
                break;
            default:
                message += "an unknown type";
                break;
            }
        }
        if (!remotePlayers[i])
        {
            std::string name (callsign);
            name += ": " + message;
            message = name;
        }
        addMessage (remotePlayers[i], message);
    }
    completer.registerWord(callsign, true /* quote spaces */);

    if (shotStats)
        shotStats->refresh();

    return remotePlayers[i];
}

bool BZFlagNew::removePlayer(PlayerId id) {
    int playerIndex = lookupPlayerIndex(id);

    if (playerIndex < 0)
        return false;

    Player* p = remotePlayers[playerIndex];

    Address addr;
    std::string msg = "signing off";
    if (!p->getIpAddress(addr))
        addMessage(p, "signing off");
    else
    {
        msg += " from ";
        msg += addr.getDotNotation();
        p->setIpAddress(addr);
        addMessage(p, msg);
        //if (BZDB.evalInt("showips") > 1)
        //    printIpInfo (p, addr, "(leave)");
    }

    if (myTank->getRecipient() == p)
        myTank->setRecipient(0);
    if (myTank->getNemesis() == p)
        myTank->setNemesis(0);

    completer.unregisterWord(p->getCallSign());

    delete remotePlayers[playerIndex];
    remotePlayers[playerIndex] = NULL;

    while ((playerIndex >= 0)
            &&     (playerIndex+1 == curMaxPlayers)
            &&     (remotePlayers[playerIndex] == NULL))
    {
        playerIndex--;
        curMaxPlayers--;
    }
    World::getWorld()->setCurMaxPlayers(curMaxPlayers);

    if (shotStats)
        shotStats->refresh();

    return true;
}

void BZFlagNew::viewportEvent(ViewportEvent& e) {
    GL::defaultFramebuffer.setViewport({{}, e.framebufferSize()});
    _imgui.relayout(Vector2{e.windowSize()}/e.dpiScaling(),
        e.windowSize(), e.framebufferSize());
    _camera->setViewport(e.windowSize());
}

void BZFlagNew::keyPressEvent(KeyEvent& event) {
    if(_imgui.handleKeyPressEvent(event)) return;
}

void BZFlagNew::keyReleaseEvent(KeyEvent& event) {
    if(_imgui.handleKeyReleaseEvent(event)) return;
}

void BZFlagNew::mousePressEvent(MouseEvent& e) {
    if(_imgui.handleMousePressEvent(e)) return;
    if (e.button() == MouseEvent::Button::Left)
        _previousPosition = positionOnSphere(e.position());
}

void BZFlagNew::mouseReleaseEvent(MouseEvent& e) {
    if(_imgui.handleMouseReleaseEvent(e)) return;
    if (e.button() == MouseEvent::Button::Left)
        _previousPosition = Vector3{};
}

void BZFlagNew::mouseScrollEvent(MouseScrollEvent& e) {
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

void BZFlagNew::mouseMoveEvent(MouseMoveEvent &e) {
    if(_imgui.handleMouseMoveEvent(e)) return;
    if (!(e.buttons() & MouseMoveEvent::Button::Left)) return;

    const Vector3 currentPosition = positionOnSphere(e.position());
    const Vector3 axis = Math::cross(_previousPosition, currentPosition);

    if (_previousPosition.length() < 0.001f || axis.length() < 0.001f) return;

    _manipulator.rotate(Math::angle(_previousPosition, currentPosition), axis.normalized());
    _previousPosition = currentPosition;

    redraw();
}

void BZFlagNew::textInputEvent(TextInputEvent& e) {
    if(_imgui.handleTextInputEvent(e)) return;
}

void BZFlagNew::exitEvent(ExitEvent& e) {
    isQuit = true;
}

void BZFlagNew::tryConnect(const std::string& callsign, const std::string& password, const std::string& server, const std::string& port)
{
    BZDB.set("callsign", callsign.c_str());
    BZDB.set("server", server.c_str());
    BZDB.set("port", port.c_str());
    if (password != BZDB.get("password")) {
        // Password has changed, token is invalid.
        startupInfo.token[0] = '\0';
        BZDB.set("password", password.c_str());
    }

    strcpy(startupInfo.callsign, callsign.c_str());
    strcpy(startupInfo.serverName, server.c_str());
    startupInfo.serverPort = std::atoi(port.c_str());
    strcpy(startupInfo.password, password.c_str());

    joinRequested = true;
}

Vector3 BZFlagNew::positionOnSphere(const Vector2i& position) const {
    const Vector2 positionNormalized = Vector2{position}/Vector2{_camera->viewport()} - Vector2{0.5f};
    const Float length = positionNormalized.length();
    const Vector3 result(length > 1.0f ? Vector3(positionNormalized, 0.0f) : Vector3(positionNormalized, 1.0f - length));
    return (result * Vector3::yScale(-1.0f)).normalized();
}

int BZFlagNew::main() {
    initAres();

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

    CommandsStandard::add();
    unsigned int i;

    initConfigData();
    loadBZDBDefaults();

    ServerListCache::get()->loadCache();

    //BZDB.set("callsign", "testingbz");
    //BZDB.set("server", "1purplepanzer.mooo.com");
    //BZDB.set("port", "4100");

    startupInfo.useUDPconnection=true;
    startupInfo.team = ObserverTeam;
    //strcpy(startupInfo.callsign, "testingbz");
    //strcpy(startupInfo.serverName, "1purplepanzer.mooo.com");
    //startupInfo.serverPort = 4100;

    //startupInfo.autoConnect = true;

    Team::updateShotColors();

    startPlaying();

    killAres();
    AresHandler::globalShutdown();
    return 0;
}

void BZFlagNew::joinInternetGame2()
{
    justJoined = true;

    //HUDDialogStack::get()->setFailedMessage("Entering game...");

    ServerLink::setServer(serverLink);
    World::setWorld(world);

    // prep teams
    teams = world->getTeams();

    // prep players
    curMaxPlayers = 0;
    remotePlayers = world->getPlayers();

    // reset the autocompleter
    completer.setDefaults();
    //BZDB.iterate(addVarToAutoComplete, NULL);

    // prep flags
    numFlags = world->getMaxFlags();

    // make scene database
    //setSceneDatabase();
    //mainWindow->getWindow()->yieldCurrent();

    worldRenderer.createWorldObject();
    worldRenderer.getWorldObject()->setParent(&_manipulator);

    // make radar
    //  radar = new RadarRenderer(*sceneRenderer, *world);
    //  mainWindow->getWindow()->yieldCurrent();
    //radar->setWorld(world);
    //controlPanel->setRadarRenderer(radar);
    //controlPanel->resize();

    // make local player
    myTank = new LocalPlayer(_serverLink->getId(), startupInfo.callsign,
                             startupInfo.motto);
    myTank->setTeam(startupInfo.team);
    LocalPlayer::setMyTank(myTank);

    if (world->allowRabbit() && myTank->getTeam() != ObserverTeam)
        myTank->setTeam(HunterTeam);

    // tell server we want to join
    _serverLink->sendEnter(TankPlayer, myTank->getTeam(),
                          myTank->getCallSign(),
                          myTank->getMotto(),
                          startupInfo.token);
    startupInfo.token[0] = '\0';

    // hopefully it worked!  pop all the menus.
    //HUDDialogStack* stack = HUDDialogStack::get();
    //while (stack->isActive())
    //    stack->pop();
    joiningGame = false;
}

ServerLink* BZFlagNew::lookupServer(const Player *_player) {
    PlayerId id = _player->getId();
    if (myTank->getId() == id) return _serverLink;
/*#ifdef ROBOT
    for (int i = 0; i < numRobots; i++)
        if (robots[i] && robots[i]->getId() == id)
            return robotServer[i];
#endif*/
    return NULL;
}

void BZFlagNew::handleFlagDropped(Player* tank) {
        // skip it if player doesn't actually have a flag
    if (tank->getFlag() == ::Flags::Null) return;

    if (tank == myTank)
    {
        // make sure the player must reload after theft
        if (tank->getFlag() == ::Flags::Thief)
            myTank->forceReload(BZDB.eval(StateDatabase::BZDB_THIEFDROPTIME));
        //drop lock if i had GM
        if (myTank->getFlag() == ::Flags::GuidedMissile)
            myTank->setTarget(NULL);

        // update display and play sound effects
        //playLocalSound(SFX_DROP_FLAG);
        updateFlag(::Flags::Null);
    }
    //else if (isViewTank(tank))
    //    playLocalSound(SFX_DROP_FLAG);

    // add message
    std::string message("dropped ");
    message += tank->getFlag()->flagName;
    message += " flag";
    addMessage(tank, message);

    // player no longer has flag
    tank->setFlag(::Flags::Null);
}

bool BZFlagNew::gotBlowedUp(BaseLocalPlayer* tank, BlowedUpReason reason, PlayerId killer, const ShotPath* hit, int phydrv) {
        if (tank && (tank->getTeam() == ObserverTeam || !tank->isAlive()))
        return false;

    int shotId = -1;
    FlagType* flagType = ::Flags::Null;
    if (hit)
    {
        shotId = hit->getShotId();
        flagType = hit->getFlag();
    }

    // you can't take it with you
    const FlagType* flag = tank->getFlag();
    if (flag != ::Flags::Null)
    {
        // TODO: Enable autopilot
        //if (myTank->isAutoPilot())
        //    teachAutoPilot( myTank->getFlag(), -1 );

        // tell other players I've dropped my flag
        lookupServer(tank)->sendDropFlag(tank->getPosition());

        // drop it
        handleFlagDropped(tank);
    }

    // restore the sound, this happens when paused tank dies
    // (genocide or team flag captured)
    if (savedVolume != -1)
    {
        //setSoundVolume(savedVolume);
        savedVolume = -1;
    }

    // take care of explosion business -- don't want to wait for
    // round trip of killed message.  waiting would simplify things,
    // but the delay (2-3 frames usually) can really fool and irritate
    // the player.  we have to be careful to ignore our own Killed
    // message when it gets back to us -- do this by ignoring killed
    // message if we're already dead.
    // don't die if we had the shield flag and we've been shot.
    if (reason != GotShot || flag != ::Flags::Shield)
    {
        // blow me up

        // todo hook this back up for 2.4.4. or later
        TankDeathOverride *death = NULL;
        //EFFECTS.addDeathEffect(tank->getColor(), tank->getPosition(), tank->getAngle(),reason,tank, flagType);

        tank->setDeathEffect(death);
        tank->explodeTank();

        /*if (isViewTank(tank))
        {
            if (reason == GotRunOver)
                playLocalSound(SFX_RUNOVER);
            else
                playLocalSound(SFX_DIE);
            ForceFeedback::death();
        }
        else*/
        {
            const float* pos = tank->getPosition();
            if (reason == GotRunOver)
            {
                //playWorldSound(SFX_RUNOVER, pos,
                //               getLocalPlayer(killer) == myTank);
            }
            else
            {
                //playWorldSound(SFX_EXPLOSION, pos,
                //               getLocalPlayer(killer) == myTank);
            }
        }

        /*if (tank != myTank &&(!death || death->ShowExplosion()))
        {
            const float* pos = tank->getPosition();
            float explodePos[3];
            explodePos[0] = pos[0];
            explodePos[1] = pos[1];
            explodePos[2] = pos[2] + tank->getMuzzleHeight();
            addTankExplosion(explodePos);
        }*/

        // tell server I'm dead in case it doesn't already know
        if (reason == GotShot || reason == GotRunOver ||
                reason == GenocideEffect || reason == SelfDestruct ||
                reason == WaterDeath || reason == DeathTouch)
            lookupServer(tank)->sendKilled(killer, reason, shotId, flagType, phydrv);
    }

    // print reason if it's my tank
    if ((tank == myTank) &&
            (((reason < LastReason) && blowedUpMessage[reason]) ||
             (reason == PhysicsDriverDeath)))
    {

        std::string blowedUpNotice;
        if (reason < LastReason)
            blowedUpNotice = blowedUpMessage[reason];
        else if (reason == PhysicsDriverDeath)
        {
            const PhysicsDriver* driver = PHYDRVMGR.getDriver(phydrv);
            if (driver)
                blowedUpNotice = driver->getDeathMsg();
            else
                blowedUpNotice = "Killed by unknown obstacle";
        }
        else
            blowedUpNotice = "Invalid reason";

        // first, check if i'm the culprit
        if (reason == GotShot && getLocalPlayer(killer) == myTank)
            blowedUpNotice = "Shot myself";
        else
        {
            // 1-4 are messages sent when the player dies because of someone else
            if (reason >= GotShot && reason <= GenocideEffect)
            {
                Player *killerPlayer = lookupPlayer(killer);
                if (!killerPlayer)
                    blowedUpNotice = "Killed by the server";
                else
                {

                    // matching the team-display style of other kill messages
                    TeamColor team = killerPlayer->getTeam();
                    if (hit)
                        team = hit->getTeam();
                    if (World::getWorld()->allowTeams() && (myTank->getTeam() == team) && (team != RogueTeam) && (team != ObserverTeam))
                    {
                        blowedUpNotice += "teammate " ;
                        blowedUpNotice += killerPlayer->getCallSign();
                    }
                    else
                    {
                        blowedUpNotice += killerPlayer->getCallSign();
                        blowedUpNotice += " (";
                        blowedUpNotice += Team::getName(killerPlayer->getTeam());
                        blowedUpNotice += ")";
                        if (flagType != ::Flags::Null)
                        {
                            blowedUpNotice += " with ";
                            blowedUpNotice += flagType->flagAbbv;
                        }
                    }
                }
            }
        }
        //hud->setAlert(0, blowedUpNotice.c_str(), 4.0f, true);
        //controlPanel->addMessage(blowedUpNotice);
    }

    // make sure shot is terminated locally (if not globally) so it can't
    // hit me again if I had the shield flag.  this is important for the
    // shots that aren't stopped by a hit and so may stick around to hit
    // me on the next update, making the shield useless.
    return (reason == GotShot && flag == ::Flags::Shield && shotId != -1);
}

const void *BZFlagNew::handleMsgSetVars(const void *msg) {
    uint16_t numVars;
    uint8_t nameLen = 0, valueLen = 0;

    char name[MaxPacketLen];
    char value[MaxPacketLen];

    msg = nboUnpackUShort(msg, numVars);
    for (int i = 0; i < numVars; i++)
    {
        msg = nboUnpackUByte(msg, nameLen);
        msg = nboUnpackString(msg, name, nameLen);
        name[nameLen] = '\0';

        msg = nboUnpackUByte(msg, valueLen);
        msg = nboUnpackString(msg, value, valueLen);
        value[valueLen] = '\0';

        if ((name[0] != '_') && (name[0] != '$'))
        {
            logDebugMessage(1, "Server BZDB change blocked: '%s' = '%s'\n",
                            name, value);
        }
        else
        {
            BZDB.set(name, value);
            BZDB.setPersistent(name, false);
            BZDB.setPermission(name, StateDatabase::Locked);
        }
    }
    return msg;
}

void BZFlagNew::handleFlagTransferred(Player *fromTank, Player *toTank, int flagIndex)
{
    ::Flag f = world->getFlag(flagIndex);

    fromTank->setFlag(::Flags::Null);
    toTank->setFlag(f.type);

    if ((fromTank == myTank) || (toTank == myTank))
        updateFlag(myTank->getFlag());

    const float *pos = toTank->getPosition();
    if (f.type->flagTeam != ::NoTeam)
    {
        /*if ((toTank->getTeam() == myTank->getTeam()) && (f.type->flagTeam != myTank->getTeam()))
            playWorldSound(SFX_TEAMGRAB, pos);
        else if ((fromTank->getTeam() == myTank->getTeam()) && (f.type->flagTeam == myTank->getTeam()))
        {
            hud->setAlert(1, "Flag Alert!!!", 3.0f, true);
            playLocalSound(SFX_ALERT);
        }*/
    }

    std::string message("stole ");
    message += fromTank->getCallSign();
    message += "'s flag";
    addMessage(toTank, message);
}

void BZFlagNew::handlePlayerMessage(uint16_t code, uint16_t, const void* msg)
{
    switch (code)
    {
    case MsgPlayerUpdate:
    case MsgPlayerUpdateSmall:
    {
        float timestamp; // could be used to enhance deadreckoning, but isn't for now
        PlayerId id;
        int32_t order;
        const void *buf = msg;
        buf = nboUnpackFloat(buf, timestamp);
        buf = nboUnpackUByte(buf, id);
        Player* tank = lookupPlayer(id);
        if (!tank || tank == myTank) break;
        nboUnpackInt(buf, order); // peek! don't update the msg pointer
        if (order <= tank->getOrder()) break;
        short oldStatus = tank->getStatus();
        tank->unpack(msg, code);
        short newStatus = tank->getStatus();
        if ((oldStatus & short(PlayerState::Paused)) !=
                (newStatus & short(PlayerState::Paused)))
            addMessage(tank, (tank->getStatus() & PlayerState::Paused) ?
                       "Paused" : "Resumed");
        if ((oldStatus & short(PlayerState::Exploding)) == 0 &&
                (newStatus & short(PlayerState::Exploding)) != 0)
        {
            // player has started exploding and we haven't gotten killed
            // message yet -- set explosion now, play sound later (when we
            // get killed message).  status is already !Alive so make player
            // alive again, then call setExplode to kill him.
            tank->setStatus(newStatus | short(PlayerState::Alive));
            tank->setExplode(TimeKeeper::getTick());
            // ROBOT -- play explosion now
        }
        break;
    }

    case MsgGMUpdate:
    {
        ShotUpdate shot;
        msg = shot.unpack(msg);
        Player* tank = lookupPlayer(shot.player);
        if (!tank || tank == myTank) break;
        RemotePlayer* remoteTank = (RemotePlayer*)tank;
        RemoteShotPath* shotPath =
            (RemoteShotPath*)remoteTank->getShot(shot.id);
        if (shotPath) shotPath->update(shot, code, msg);
        PlayerId targetId;
        msg = nboUnpackUByte(msg, targetId);
        Player* targetTank = lookupPlayer(targetId);
        if (targetTank && (targetTank == myTank) && (myTank->isAlive()))
        {
            static TimeKeeper lastLockMsg;
            if (TimeKeeper::getTick() - lastLockMsg > 0.75)
            {
                //playWorldSound(SFX_LOCK, shot.pos);
                lastLockMsg=TimeKeeper::getTick();
                addMessage(tank, "locked on me");
            }
        }
        break;
    }

    // just echo lag ping message
    case MsgLagPing:
        _serverLink->send(MsgLagPing,2,msg);
        break;
    }
}

void BZFlagNew::startupErrorCallback(const char* msg)
{
    Warning{} << msg;
}

void BZFlagNew::startPlaying()
{
    lastObserverUpdateTime = TimeKeeper::getTick().getSeconds();

    // register some commands
    for (unsigned int c = 0; c < bzcountof(commandList); ++c)
        CMDMGR.add(commandList[c].name, commandList[c].func, commandList[c].help);

    // init tank display lists
    // make control panel
    // make radar
    // tie radar to ctrl panel

    setErrorCallback(startupErrorCallback);

    // init epoch offset and update daylight

    // signal handling

    // make hud
    // init ctrl panel and hud
    // make bg renderer
    // load explosion textures and make explosion scene nodes
    // make scenedatabasebuilder and init world

    World::init();

    // prepare dialogs / main menu

    setErrorCallback(startupErrorCallback);

        // print debugging info
    {
        // Application version
        logDebugMessage(1,"BZFlag version:   %s\n", getAppVersion());

        // Protocol version
        logDebugMessage(1,"BZFlag protocol:  %s\n", getProtocolVersion());

        // OpenGL Driver Information
        logDebugMessage(1,"OpenGL vendor:    %s\n", (const char*)glGetString(GL_VENDOR));
        logDebugMessage(1,"OpenGL version:   %s\n", (const char*)glGetString(GL_VERSION));
        logDebugMessage(1,"OpenGL renderer:  %s\n", (const char*)glGetString(GL_RENDERER));
    }

    if (!BZDB.isTrue("disableMOTD"))
    {
        motd = new MessageOfTheDay;
        motd->getURL(BZDB.get("motdServer"));
    }

    if (startupInfo.autoConnect &&
            startupInfo.callsign[0] && startupInfo.serverName[0])
    {
        joinRequested    = true;
    }

    TimeKeeper::setTick();

    worldDownLoader = new WorldDownLoader(this);

    playingLoop();

    delete worldDownLoader;

    leaveGame();
    World::done();
}

void BZFlagNew::playingLoop()
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

        // see if the world collision grid needs to be updated
        if (world)
            world->checkCollisionManager();

        // try to join a game if requested.  do this *before* handling
        // events so we do a redraw after the request is posted and
        // before we actually try to join.
        if (joinRequested)
        {
            // if already connected to a game then first sign off
            if (myTank) leaveGame();

            // get token if we need to (have a password but no token)
            if ((startupInfo.token[0] == '\0')
                    && (startupInfo.password[0] != '\0'))
            {
                ServerList* serverList = new ServerList;
                serverList->startServerPings(&startupInfo);
                // wait no more than 10 seconds for a token
                for (int j = 0; j < 40; j++)
                {
                    serverList->checkEchos(&startupInfo);
                    cURLManager::perform();
                    if (startupInfo.token[0] != '\0') break;
                    TimeKeeper::sleep(0.25f);
                }
                delete serverList;
            }
            ares->queryHost(startupInfo.serverName);
            waitingDNS = true;

            // don't try again
            joinRequested = false;
        }

        if (waitingDNS)
        {
            fd_set readers, writers;
            int nfds = -1;
            struct timeval timeout;
            timeout.tv_sec  = 0;
            timeout.tv_usec = 0;
            FD_ZERO(&readers);
            FD_ZERO(&writers);
            ares->setFd(&readers, &writers, nfds);
            nfds = select(nfds + 1, (fd_set*)&readers, (fd_set*)&writers, 0,
                          &timeout);
            ares->process(&readers, &writers);

            struct in_addr inAddress;
            AresHandler::ResolutionStatus status = ares->getHostAddress(&inAddress);
            if (status == AresHandler::Failed)
            {
                //HUDDialogStack::get()->setFailedMessage("Server not found");
                //std::cout << "Ares Handler failed, server not found." << std::endl;
                console.addLog("Server not found");
                waitingDNS = false;
            }
            else if (status == AresHandler::HbNSucceeded)
            {
                // now try connecting
                serverNetworkAddress = Address(inAddress);
                char buf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &inAddress.s_addr, buf, sizeof(buf));
                joinInternetGame();
                waitingDNS = false;
            }
        }
        // Process input events for a short time (base this on processInputEvents(maxproctime))
        // hasjoystick handling


        // Call playing callbacks
        {
            const int count = playingCallbacks.size();
            for (int i = 0; i < count; i++)
            {
                const PlayingCallbackItem& cb = playingCallbacks[i];
                (*cb.cb)(cb.data);
            }
        }

        // quick out
        if (CommandsStandard::isQuit())
            break;
        
        // if server died then leave the game (note that this may cause
        // further server errors but that's okay).
        if (serverError ||
                (_serverLink && _serverLink->getState() == ServerLink::Hungup))
        {
            // if we haven't reported the death yet then do so now
            if (serverDied ||
                    (_serverLink && _serverLink->getState() == ServerLink::Hungup))
                printError("Server has unexpectedly disconnected");
            leaveGame();
        }

        // TODO: Update sky every few seconds
        if (world)
            world->updateWind(dt);

        // Move roaming camera

        // update test video format timer

        // update the countdowns
        //updatePauseCountdown(dt);
        //updateDestructCountdown(dt);

        // update other tank's shots
        for (int i = 0; i < curMaxPlayers; i++)
        {
            if (remotePlayers[i])
                remotePlayers[i]->updateShots(dt);
        }

        const World *_world = World::getWorld();
        if (_world)
            _world->getWorldWeapons()->updateShots(dt);
        // update track marks  (before any tanks are moved)
        //TrackMarks::update(dt);

        // do dead reckoning on remote players
        for (int i = 0; i < curMaxPlayers; i++)
        {
            if (remotePlayers[i])
            {
                const bool wasNotResponding = remotePlayers[i]->isNotResponding();
                remotePlayers[i]->doDeadReckoning();
                const bool isNotResponding = remotePlayers[i]->isNotResponding();
                if (!wasNotResponding && isNotResponding)
                    addMessage(remotePlayers[i], "not responding");
                else if (wasNotResponding && !isNotResponding)
                    addMessage(remotePlayers[i], "okay");
            }
        }

        // do motion
        if (myTank)
        {
            if (myTank->isAlive() && !myTank->isPaused())
            {
                doMotion();
                /*if (scoreboard->getHuntState()==ScoreboardRenderer::HUNT_ENABLED)
                {
                    setHuntTarget(); //spot hunt target
                }*/
                if (myTank->getTeam() != ObserverTeam &&
                        ((fireButton && myTank->getFlag() == ::Flags::MachineGun) ||
                         (myTank->getFlag() == ::Flags::TriggerHappy)))
                    myTank->fireShot();

                //setLookAtMarker();

                // see if we have a target, if so lock on to the bastage
                const Player* targetdPlayer = myTank->getTarget();
                if (targetdPlayer && targetdPlayer->isAlive() && targetdPlayer->getFlag() != ::Flags::Stealth)
                {
                    hud->AddLockOnMarker(Float3ToVec3(myTank->getTarget()->getPosition()),
                                         myTank->getTarget()->getCallSign(),
                                         !isKillable(myTank->getTarget()));
                }
                else // if we should not have a target, force that target to be cleared
                    myTank->setTarget(NULL);

            }
            else
            {
                //int mx, my;
                //mainWindow->getMousePosition(mx, my);
            }
            myTank->update();
        }

        checkEnvironment();

        // adjust properties based on flags (dimensions, cloaking, etc...)
        if (myTank)
            myTank->updateTank(dt, true);
        for (int i = 0; i < curMaxPlayers; i++)
        {
            if (remotePlayers[i])
                remotePlayers[i]->updateTank(dt, false);
        }

        updateFlags(dt);
        // updateExplosions(dt): Update billboard scene nodes for explosions
                // update mesh animations
        if (world)
            world->updateAnimations(dt);

        // draw frame
        mainLoopIteration();

        // updateSound()

        bool sendUpdate = myTank && myTank->isDeadReckoningWrong();
        if (myTank && myTank->getTeam() == ObserverTeam)
        {
            if (BZDB.isTrue("sendObserverHeartbeat"))
            {
                double heartbeatTime = BZDB.isSet("observerHeartbeat")
                                       ? BZDB.eval("observerHeartbeat") : 30.0f;
                if (lastObserverUpdateTime + heartbeatTime < TimeKeeper::getTick().getSeconds())
                {
                    lastObserverUpdateTime = TimeKeeper::getTick().getSeconds();
                    sendUpdate = true;
                }
                else
                    sendUpdate = false;
            }
            else
                sendUpdate = false;
        }
        // send my data
        if (sendUpdate)
        {
            // also calls setDeadReckoning()
            _serverLink->sendPlayerUpdate(myTank);
        }

        cURLManager::perform();

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
            //else
                //setSceneDatabase();
        }

        // FPS limit: saveEnergy BZDB

        // handle incoming packets
        doMessages();
    }
    std::cout << "Done playing" << std::endl;

}

void BZFlagNew::doMessages() {
    char msg[MaxPacketLen];
    uint16_t code, len;
    int e = 0;
    // handle server messages
    if (_serverLink)
    {

        while (!serverError && (e = _serverLink->read(code, len, msg, 0)) == 1)
            handleServerMessage(true, code, len, msg);
        if (e == -2)
        {
            printError("Server communication error");
            serverError = true;
            return;
        }
    }
}

void BZFlagNew::handleServerMessage(bool human, uint16_t code, uint16_t len, const void* msg) {
        bool checkScores = false;
    static WordFilter *wordfilter = (WordFilter *)BZDB.getPointer("filter");

    switch (code)
    {

    case MsgNearFlag:
        // MsgNearFlag may arrive up to 1 lag period after dropping ID,
        // so process this only when carrying the ID flag
        if (myTank && myTank->getFlag() == ::Flags::Identify)
            //handleNearFlag(msg,len);
        break;

    case MsgFetchResources:
        if (BZDB.isSet ("_noRemoteFiles") && BZDB.isTrue ("_noRemoteFiles"))
            break;
        else
        {
            uint16_t numItems;
            const void *buf;

            buf = nboUnpackUShort (msg, numItems);    // the type

            for (int i = 0; i < numItems; i++)
            {
                uint16_t itemType;
                char buffer[MessageLen];
                uint16_t stringLen;
                trResourceItem item;

                buf = nboUnpackUShort (buf, itemType);
                item.resType = (teResourceType) itemType;

                // URL
                buf = nboUnpackUShort (buf, stringLen);
                buf = nboUnpackString (buf, buffer, stringLen);

                buffer[stringLen] = '\0';
                item.URL = buffer;

                item.filePath = PlatformFactory::getMedia ()->getMediaDirectory() + DirectorySeparator;
                std::vector < std::string > temp =
                    TextUtils::tokenize (item.URL, std::string ("/"));

                item.fileName = temp[temp.size () - 1];
                item.filePath += item.fileName;

                std::string hostname;
                /*parseHostname (item.URL, hostname);
                if (authorizedServer (hostname))
                {
                    if (!resourceDownloader)
                        resourceDownloader = new ResourceGetter;
                    resourceDownloader->addResource (item);
                }*/
            }
        }
        break;

    case MsgCustomSound:
        // bail out if we don't want to do remote sounds
        if (BZDB.isSet ("_noRemoteSounds") && BZDB.isTrue ("_noRemoteSounds"))
            break;
        else
        {
            const void *buf;
            char buffer[MessageLen];
            uint16_t soundType;
            uint16_t stringLen;
            std::string soundName;

            buf = nboUnpackUShort (msg, soundType);   // the type
            buf = nboUnpackUShort (buf, stringLen);   // how long our str is
            buf = nboUnpackString (buf, buffer, stringLen);

            buffer[stringLen] = '\0';
            soundName = buffer;

            //if (soundType == LocalCustomSound)
                //playLocalSound (soundName);
        }
        break;

    case MsgUDPLinkEstablished:
        // server got our initial UDP packet
        _serverLink->enableOutboundUDP();
        break;

    case MsgUDPLinkRequest:
        // we got server's initial UDP packet
        _serverLink->confirmIncomingUDP();
        break;

    case MsgSuperKill:
        printError("Server forced a disconnect");
        serverError = true;
        break;

    case MsgAccept:
        break;

    case MsgReject:
    {
        const void *buf;
        char buffer[MessageLen];
        uint16_t rejcode;
        std::string reason;
        buf = nboUnpackUShort (msg, rejcode); // filler for now
        buf = nboUnpackString (buf, buffer, MessageLen);
        buffer[MessageLen - 1] = '\0';
        reason = buffer;
        printError(reason);
        serverError = true;
        break;
    }

    case MsgNegotiateFlags:
    {
        if (len > 0)
        {
            //dumpMissingFlag((const char *)msg, len);
            break;
        }
        _serverLink->send(MsgWantSettings, 0, NULL);
        break;
    }

    case MsgGameSettings:
    {
        if (worldBuilder)
            delete worldBuilder;
        worldBuilder = new WorldBuilder;
        worldBuilder->unpackGameSettings(msg);
        _serverLink->send(MsgWantWHash, 0, NULL);
        //HUDDialogStack::get()->setFailedMessage("Requesting World Hash...");
        //std::cout << "Requesting world hash..." << std::endl;
        console.addLog("Requesting world hash...");
        break;
    }

    case MsgCacheURL:
    {
        char *cacheURL = new char[len];
        nboUnpackString(msg, cacheURL, len);
        worldUrl = cacheURL;
        delete [] cacheURL;
        break;
    }

    case MsgWantWHash:
    {
        char *hexDigest = new char[len];
        nboUnpackString(msg, hexDigest, len);
        isCacheTemp = hexDigest[0] == 't';
        md5Digest = &hexDigest[1];

        worldDownLoader->start(hexDigest);
        delete [] hexDigest;
        break;
    }

    case MsgGetWorld:
    {
        // create world
        uint32_t bytesLeft;
        const void *buf = nboUnpackUInt(msg, bytesLeft);
        bool last = processWorldChunk(buf, len - 4, bytesLeft);
        if (!last)
        {
            char message[MaxPacketLen];
            // ask for next chunk
            worldPtr += len - 4;
            nboPackUInt(message, worldPtr);
            _serverLink->send(MsgGetWorld, sizeof(uint32_t), message);
            break;
        }
        if (cacheOut)
            delete cacheOut;
        cacheOut = NULL;
        
        loadCachedWorld();

        if (isCacheTemp)
            markOld(worldCachePath);
        break;
    }

    case MsgGameTime:
    {
        GameTime::unpack(msg);
        GameTime::update();
        break;
    }

    case MsgTimeUpdate:
    {
        int32_t timeLeft;
        msg = nboUnpackInt(msg, timeLeft);
        //hud->setTimeLeft(timeLeft);
        if (timeLeft == 0)
        {
            if (myTank->getTeam() != ObserverTeam)
                gameOver = true;
            myTank->explodeTank();
            //controlPanel->addMessage("Time Expired");
            //hud->setAlert(0, "Time Expired", 10.0f, true);
            //controlPanel->addMessage("GAME OVER");
            //hud->setAlert(1, "GAME OVER", 10.0f, true);
        }
        else if (timeLeft < 0)
        {
            //std::cout << "Game Paused" << std::endl;
            //controlPanel->addMessage("Game Paused");
            //hud->setAlert(0, "Game Paused", 10.0f, true);
            console.addLog("Game Paused");
        }
        break;
    }

    case MsgScoreOver:
    {
        // unpack packet
        PlayerId id;
        uint16_t team;
        msg = nboUnpackUByte(msg, id);
        msg = nboUnpackUShort(msg, team);
        Player* _player = lookupPlayer(id);

        // make a message
        std::string msg2;
        if (team == (uint16_t)NoTeam)
        {
            // a player won
            if (_player)
            {
                msg2 = _player->getCallSign();
                msg2 += " (";
                msg2 += Team::getName(_player->getTeam());
                msg2 += ")";
            }
            else
                msg2 = "[unknown player]";
        }
        else
        {
            // a team won
            msg2 = Team::getName(TeamColor(team));
        }
        msg2 += " won the game";

        if (myTank->getTeam() != ObserverTeam)
            gameOver = true;
        //hud->setTimeLeft((uint32_t)~0);
        myTank->explodeTank();
        //controlPanel->addMessage(msg2);
        //hud->setAlert(0, msg2.c_str(), 10.0f, true);
        //std::cout << msg2 << std::endl;
        console.addLog(msg2.c_str());
        break;
    }

    case MsgAddPlayer:
    {
        PlayerId id;
        msg = nboUnpackUByte(msg, id);
#if defined(FIXME) && defined(ROBOT)
        saveRobotInfo(id, msg);
#endif
        if (id == myTank->getId())
        {
            // it's me!  should be the end of updates
            enteringServer(msg);
        }
        else
        {
            addPlayer(id, msg, entered);
            checkScores = true;

            // update the tank flags when in replay mode.
            if (myTank->getId() >= 200)
                setTankFlags();
        }
        break;
    }

    case MsgRemovePlayer:
    {
        PlayerId id;
        msg = nboUnpackUByte(msg, id);
        if (removePlayer (id))
            checkScores = true;
        break;
    }

    case MsgFlagUpdate:
    {
        uint16_t count;
        uint16_t flagIndex;
        uint32_t offset = 0;
        msg = nboUnpackUShort(msg, count);
        offset += 2;
        for (int i = 0; i < count; i++)
        {
            if (offset >= len)
                break;

            msg = nboUnpackUShort(msg, flagIndex);
            msg = world->getFlag(int(flagIndex)).unpack(msg);
            offset += FlagPLen;

            world->initFlag(int(flagIndex));
        }
        break;
    }

    case MsgTeamUpdate:
    {
        uint8_t  numTeams;
        uint16_t team;

        msg = nboUnpackUByte(msg,numTeams);
        for (int i = 0; i < numTeams; i++)
        {
            msg = nboUnpackUShort(msg, team);
            msg = teams[int(team)].unpack(msg);
        }
        checkScores = true;
        break;
    }

    case MsgAlive:
    {
        PlayerId id;
        float pos[3], forward;
        msg = nboUnpackUByte(msg, id);
        msg = nboUnpackVector(msg, pos);
        msg = nboUnpackFloat(msg, forward);
        int playerIndex = lookupPlayerIndex(id);

        if ((playerIndex >= 0) || (playerIndex == -2))
        {
            static const float zero[3] = { 0.0f, 0.0f, 0.0f };
            Player* tank = getPlayerByIndex(playerIndex);
            if (tank == myTank)
            {
                wasRabbit = tank->getTeam() == RabbitTeam;
                myTank->restart(pos, forward);
                firstLife = false;
                justJoined = false;
                if (!myTank->isAutoPilot())
                    mainWindow->warpMouse();
                //hud->setAltitudeTape(World::getWorld()->allowJumping());
            }

            tank->setDeathEffect(NULL);
            if (SceneRenderer::instance().useQuality() >= 2)
            {
                if (((tank != myTank)
                        && ((ROAM.getMode() != Roaming::roamViewFP)
                            || (tank != ROAM.getTargetTank())))
                        || BZDB.isTrue("enableLocalSpawnEffect"))
                {
                    /*if (myTank->getFlag() == Flags::Colorblindness)
                    {
                        static float cbColor[4] = {1,1,1,1};
                        EFFECTS.addSpawnEffect(cbColor, pos);
                    }
                    else
                        EFFECTS.addSpawnEffect(tank->getColor(), pos);*/
                }
            }
            tank->setStatus(PlayerState::Alive);
            tank->move(pos, forward);
            tank->setVelocity(zero);
            tank->setAngularVelocity(0.0f);
            tank->setDeadReckoning();
            tank->spawnEffect();
            if (tank == myTank)
                myTank->setSpawning(false);
            //playSound(SFX_POP, pos, true, isViewTank(tank));
        }

        break;
    }

    case MsgAutoPilot:
    {
        PlayerId id;
        msg = nboUnpackUByte(msg, id);
        uint8_t autopilot;
        nboUnpackUByte(msg, autopilot);
        Player* tank = lookupPlayer(id);
        if (!tank) break;
        tank->setAutoPilot(autopilot != 0);
        addMessage(tank, autopilot ? "Roger taking controls" : "Roger releasing controls");
        break;
    }

    case MsgPause:
    {
        PlayerId id;
        msg = nboUnpackUByte(msg, id);
        uint8_t Pause;
        nboUnpackUByte(msg, Pause);
        Player* tank = lookupPlayer(id);
        if (!tank)
            break;

        tank->setPausedMessageState(Pause == 0);
        addMessage(tank, Pause ? "has paused" : "has unpaused" );
        break;
    }

    case MsgKilled:
    {
        PlayerId victim, killer;
        FlagType* flagType;
        int16_t shotId, reason;
        int phydrv = -1;
        msg = nboUnpackUByte(msg, victim);
        msg = nboUnpackUByte(msg, killer);
        msg = nboUnpackShort(msg, reason);
        msg = nboUnpackShort(msg, shotId);
        msg = FlagType::unpack(msg, flagType);
        if (reason == (int16_t)PhysicsDriverDeath)
        {
            int32_t inPhyDrv;
            msg = nboUnpackInt(msg, inPhyDrv);
            phydrv = int(inPhyDrv);
        }
        BaseLocalPlayer* victimLocal = getLocalPlayer(victim);
        BaseLocalPlayer* killerLocal = getLocalPlayer(killer);
        Player* victimPlayer = lookupPlayer(victim);
        Player* killerPlayer = lookupPlayer(killer);

        if (victimPlayer)
            victimPlayer->reportedHits++;
#ifdef ROBOT
        if (victimPlayer == myTank)
        {
            // uh oh, i'm dead
            if (myTank->isAlive())
            {
                _serverLink->sendDropFlag(myTank->getPosition());
                handleMyTankKilled(reason);
            }
        }
#endif
        if (victimLocal)
        {
            // uh oh, local player is dead
            if (victimLocal->isAlive())
                gotBlowedUp(victimLocal, GotKilledMsg, killer);
        }
        else if (victimPlayer)
        {
            victimPlayer->setExplode(TimeKeeper::getTick());
            const float* pos = victimPlayer->getPosition();
            //const bool localView = isViewTank(victimPlayer);
            /*if (reason == GotRunOver)
                playSound(SFX_RUNOVER, pos, killerLocal == myTank, localView);
            else
                playSound(SFX_EXPLOSION, pos, killerLocal == myTank, localView);*/
            float explodePos[3];
            explodePos[0] = pos[0];
            explodePos[1] = pos[1];
            explodePos[2] = pos[2] + victimPlayer->getMuzzleHeight();

            // TODO hook this back up for 2.4.4 or later
            TankDeathOverride* death = NULL;
            //EFFECTS.addDeathEffect(victimPlayer->getColor(), pos, victimPlayer->getAngle(),reason,victimPlayer,flagType);

            victimPlayer->setDeathEffect(death);

            /*if (!death || death->ShowExplosion())
                addTankExplosion(explodePos);*/
        }

        if (killerLocal)
        {
            // local player did it
            if (shotId >= 0)
            {
                // terminate the shot
                killerLocal->endShot(shotId, true);
            }
            if (victimPlayer && killerLocal != victimPlayer)
            {
                if ((victimPlayer->getTeam() == killerLocal->getTeam()) &&
                        (killerLocal->getTeam() != RogueTeam) && !(killerPlayer == myTank && wasRabbit)
                        && World::getWorld()->allowTeams())
                {
                    // teamkill
                    if (killerPlayer == myTank)
                    {
                        //hud->setAlert(1, "Don't kill teammates!!!", 3.0f, true);
                        //playLocalSound(SFX_KILL_TEAM);
                        //sendMeaCulpa(victimPlayer->getId());
                    }
                }
                else
                {
                    // enemy
                    if (myTank->isAutoPilot())
                    {
                        if (killerPlayer)
                        {
                            const ShotPath* shot = killerPlayer->getShot(int(shotId));
                            //if (shot != NULL)
                            //    teachAutoPilot(shot->getFlag(), 1);
                        }
                    }
                }
            }
        }
        else if (killerPlayer)
        {
            const ShotPath* shot = killerPlayer->getShot(int(shotId));
            if (shot && !shot->isStoppedByHit())
                killerPlayer->addHitToStats(shot->getFlag());
        }

        // handle my personal score against other players
        if ((killerPlayer == myTank || victimPlayer == myTank) &&
                !(killerPlayer == myTank && victimPlayer == myTank))
        {
            if (killerLocal == myTank)
            {
                if (victimPlayer)
                    victimPlayer->changeLocalScore(1, 0, 0);
                myTank->setNemesis(victimPlayer);
            }
            else
            {
                if (killerPlayer)
                    killerPlayer->changeLocalScore(0, 1, killerPlayer->getTeam() == victimPlayer->getTeam() ? 1 : 0);
                myTank->setNemesis(killerPlayer);
            }
        }

        // handle self-destructions
        if (killerPlayer == victimPlayer && killerPlayer)
            killerPlayer->changeSelfKills(1);

        // add message
        if (human && victimPlayer)
        {
            std::string message(ColorStrings[WhiteColor]);
            if (killerPlayer == victimPlayer)
            {
                message += "blew myself up";
                addMessage(victimPlayer, message);
            }
            else if (killer >= LastRealPlayer)
            {
                message += "destroyed by the server";
                addMessage(victimPlayer, message);
            }
            else if (!killerPlayer)
            {
                message += "destroyed by a (GHOST)";
                addMessage(victimPlayer, message);
            }
            else if (reason == WaterDeath)
            {
                message += "fell in the water";
                addMessage(victimPlayer, message);
            }
            else if (reason == PhysicsDriverDeath)
            {
                const PhysicsDriver* driver = PHYDRVMGR.getDriver(phydrv);
                if (driver == NULL)
                    message += "Unknown Deadly Obstacle";
                else
                    message += driver->getDeathMsg();
                addMessage(victimPlayer, message);
            }
            else
            {
                std::string playerStr;
                if (World::getWorld()->allowTeams() && (killerPlayer->getTeam() == victimPlayer->getTeam()) &&
                        (killerPlayer->getTeam() != RogueTeam) &&
                        (killerPlayer->getTeam() != ObserverTeam))
                    playerStr += "teammate ";
                if (victimPlayer == myTank)
                {
                    if (BZDB.get("killerhighlight") == "1")
                        playerStr += ColorStrings[PulsatingColor];
                    else if (BZDB.get("killerhighlight") == "2")
                        playerStr += ColorStrings[UnderlineColor];
                }
                TeamColor color = killerPlayer->getTeam();
                playerStr += Team::getAnsiCode(color);
                playerStr += killerPlayer->getCallSign();

                if (victimPlayer == myTank)
                    playerStr += ColorStrings[ResetColor];
                playerStr += ColorStrings[WhiteColor];

                // Give more informative kill messages
                if (flagType == ::Flags::Laser)
                    message += "was fried by " + playerStr + "'s laser";
                else if (flagType == ::Flags::GuidedMissile)
                    message += "was destroyed by " + playerStr + "'s guided missile";
                else if (flagType == ::Flags::ShockWave)
                    message += "felt the effects of " + playerStr + "'s shockwave";
                else if (flagType == ::Flags::InvisibleBullet)
                    message += "didn't see " + playerStr + "'s bullet";
                else if (flagType == ::Flags::MachineGun)
                    message += "was turned into swiss cheese by " + playerStr + "'s machine gun";
                else if (flagType == ::Flags::SuperBullet)
                    message += "got skewered by " + playerStr + "'s super bullet";
                else
                    message += "killed by " + playerStr;
                addMessage(victimPlayer, message, 3, killerPlayer==myTank);
            }
        }

        if (World::getWorld()->allowTeams())  // geno only works in team games :)
        {
            // blow up if killer has genocide flag and i'm on same team as victim
            // (and we're not rogues, unless in rabbit mode)
            if (human && killerPlayer && victimPlayer && victimPlayer != myTank &&
                    (victimPlayer->getTeam() == myTank->getTeam()) &&
                    (myTank->getTeam() != RogueTeam) && shotId >= 0)
            {
                // now see if shot was fired with a GenocideFlag
                const ShotPath* shot = killerPlayer->getShot(int(shotId));
                if (shot && shot->getFlag() == ::Flags::Genocide)
                    gotBlowedUp(myTank, GenocideEffect, killerPlayer->getId());
            }

#ifdef ROBOT
            // blow up robots on victim's team if shot was genocide
            if (killerPlayer && victimPlayer && shotId >= 0)
            {
                const ShotPath* shot = killerPlayer->getShot(int(shotId));
                if (shot && shot->getFlag() == Flags::Genocide)
                    for (int i = 0; i < numRobots; i++)
                        if (robots[i] && victimPlayer != robots[i] &&
                                victimPlayer->getTeam() == robots[i]->getTeam() &&
                                robots[i]->getTeam() != RogueTeam)
                            gotBlowedUp(robots[i], GenocideEffect, killerPlayer->getId());
            }
#endif
        }

        checkScores = true;
        break;
    }

    case MsgGrabFlag:
    {
        // ROBOT -- FIXME -- robots don't grab flag at the moment
        PlayerId id;
        uint16_t flagIndex;
        msg = nboUnpackUByte(msg, id);
        msg = nboUnpackUShort(msg, flagIndex);
        msg = world->getFlag(int(flagIndex)).unpack(msg);
        Player* tank = lookupPlayer(id);
        if (!tank) break;

        // player now has flag
        tank->setFlag(world->getFlag(flagIndex).type);
        if (tank == myTank)
        {
            // grabbed flag
            /*playLocalSound(myTank->getFlag()->endurance != FlagSticky ?
                           SFX_GRAB_FLAG : SFX_GRAB_BAD); */
            updateFlag(myTank->getFlag());
        }
        /*else if (isViewTank(tank))
        {
            playLocalSound(tank->getFlag()->endurance != FlagSticky ?
                           SFX_GRAB_FLAG : SFX_GRAB_BAD);
        }*/
        else if (myTank->getTeam() != RabbitTeam && tank &&
                 tank->getTeam() != myTank->getTeam() &&
                 world->getFlag(flagIndex).type->flagTeam == myTank->getTeam())
        {
            //hud->setAlert(1, "Flag Alert!!!", 3.0f, true);
            //playLocalSound(SFX_ALERT);
        }
        else
        {
            FlagType* fd = world->getFlag(flagIndex).type;
            if ( fd->flagTeam != NoTeam
                    && fd->flagTeam != tank->getTeam()
                    && ((tank && (tank->getTeam() == myTank->getTeam())))
                    && (Team::isColorTeam(myTank->getTeam())))
            {
                //hud->setAlert(1, "Team Grab!!!", 3.0f, false);
                //const float* pos = tank->getPosition();
                //playWorldSound(SFX_TEAMGRAB, pos, false);
            }
        }
        std::string message("grabbed ");
        message += tank->getFlag()->flagName;
        message += " flag";
        addMessage(tank, message);
        break;
    }

    case MsgDropFlag:
    {
        PlayerId id;
        uint16_t flagIndex;
        msg = nboUnpackUByte(msg, id);
        msg = nboUnpackUShort(msg, flagIndex);
        msg = world->getFlag(int(flagIndex)).unpack(msg);
        Player* tank = lookupPlayer(id);
        if (!tank) break;
        handleFlagDropped(tank);
        break;
    }

    case MsgCaptureFlag:
    {
        PlayerId id;
        uint16_t flagIndex, team;
        msg = nboUnpackUByte(msg, id);
        msg = nboUnpackUShort(msg, flagIndex);
        msg = nboUnpackUShort(msg, team);
        Player* capturer = lookupPlayer(id);
        if (flagIndex >= world->getMaxFlags())
            break;
        ::Flag capturedFlag = world->getFlag(int(flagIndex));
        if (capturedFlag.type == ::Flags::Null)
            break;
        int capturedTeam = world->getFlag(int(flagIndex)).type->flagTeam;

        // player no longer has flag
        if (capturer)
        {
            capturer->setFlag(::Flags::Null);
            if (capturer == myTank)
                updateFlag(::Flags::Null);

            // add message
            if (int(capturer->getTeam()) == capturedTeam)
            {
                std::string message("took my flag into ");
                message += Team::getName(TeamColor(team));
                message += " territory";
                addMessage(capturer, message);
                if (capturer == myTank)
                {
                    //hud->setAlert(1, "Don't capture your own flag!!!", 3.0f, true);
                    //playLocalSound( SFX_KILL_TEAM );
                    //sendMeaCulpa(TeamToPlayerId(myTank->getTeam()));
                }
            }
            else
            {
                std::string message("captured ");
                message += Team::getName(TeamColor(capturedTeam));
                message += "'s flag";
                addMessage(capturer, message);
            }
        }

        // play sound -- if my team is same as captured flag then my team lost,
        // but if I'm on the same team as the capturer then my team won.
        /*if (capturedTeam == int(myTank->getTeam()))
            playLocalSound(SFX_LOSE);
        else if (capturer && capturer->getTeam() == myTank->getTeam())
            playLocalSound(SFX_CAPTURE);*/


        // blow up if my team flag captured
        if (capturedTeam == int(myTank->getTeam()))
            gotBlowedUp(myTank, GotCaptured, id);
#ifdef ROBOT
        //kill all my robots if they are on the captured team
        for (int r = 0; r < numRobots; r++)
        {
            if (robots[r] && robots[r]->getTeam() == capturedTeam)
                gotBlowedUp(robots[r], GotCaptured, robots[r]->getId());
        }
#endif

        // everybody who's alive on capture team will be blowing up
        // but we're not going to get an individual notification for
        // each of them, so add an explosion for each now.  don't
        // include me, though;  I already blew myself up.
        for (int i = 0; i < curMaxPlayers; i++)
        {
            if (remotePlayers[i] &&
                    remotePlayers[i]->isAlive() &&
                    remotePlayers[i]->getTeam() == capturedTeam)
            {
                const float* pos = remotePlayers[i]->getPosition();
                //playWorldSound(SFX_EXPLOSION, pos, false);
                float explodePos[3];
                explodePos[0] = pos[0];
                explodePos[1] = pos[1];
                explodePos[2] = pos[2] + remotePlayers[i]->getMuzzleHeight();

                // todo hook this back up for 2.4.4. or later
                TankDeathOverride *death = NULL;
                /*EFFECTS.addDeathEffect(remotePlayers[i]->getColor(), pos, remotePlayers[i]->getAngle(),GotCaptured,remotePlayers[i],
                                       NULL);*/

                remotePlayers[i]->setDeathEffect(death);

                /*if (!death || death->ShowExplosion())
                    addTankExplosion(explodePos);*/
            }
        }

        checkScores = true;
        break;
    }

    case MsgNewRabbit:
    {
        PlayerId id;
        msg = nboUnpackUByte(msg, id);
        Player *rabbit = lookupPlayer(id);

        for (int i = 0; i < curMaxPlayers; i++)
        {
            if (remotePlayers[i])
                remotePlayers[i]->setHunted(false);
            if (i != id && remotePlayers[i] && remotePlayers[i]->getTeam() != HunterTeam
                    && remotePlayers[i]->getTeam() != ObserverTeam)
                remotePlayers[i]->changeTeam(HunterTeam);
        }

        if (rabbit != NULL)
        {
            rabbit->changeTeam(RabbitTeam);
            if (rabbit == myTank)
            {
                wasRabbit = true;
                if (myTank->isPaused())
                    _serverLink->sendNewRabbit();
                else
                {
                    //hud->setAlert(0, "You are now the rabbit.", 10.0f, false);
                    //playLocalSound(SFX_HUNT_SELECT);
                }
                //scoreboard->setHuntState(ScoreboardRenderer::HUNT_NONE);
            }
            else if (myTank->getTeam() != ObserverTeam)
            {
                myTank->changeTeam(HunterTeam);
                if (myTank->isPaused() || myTank->isAlive())
                    wasRabbit = false;
                rabbit->setHunted(true);
                //scoreboard->setHuntState(ScoreboardRenderer::HUNT_ENABLED);
            }

            addMessage(rabbit, "is now the rabbit", 3, true);
        }

#ifdef ROBOT
        for (int r = 0; r < numRobots; r++)
            if (robots[r])
            {
                if (robots[r]->getId() == id)
                    robots[r]->changeTeam(RabbitTeam);
                else
                    robots[r]->changeTeam(HunterTeam);
            }
#endif
        break;
    }

    case MsgShotBegin:
    {
        FiringInfo firingInfo;
        msg = firingInfo.unpack(msg);

        const int shooterid = firingInfo.shot.player;
        RemotePlayer* shooter = remotePlayers[shooterid];

        if (shooterid != ServerPlayer)
        {
            if (shooter && remotePlayers[shooterid]->getId() == shooterid)
            {
                shooter->addShot(firingInfo);

                if (SceneRenderer::instance().useQuality() >= 2)
                {
                    float shotPos[3];
                    shooter->getMuzzle(shotPos);

                    // if you are driving with a tank in observer mode
                    // and do not want local shot effects,
                    // disable shot effects for that specific tank
                    if ((ROAM.getMode() != Roaming::roamViewFP)
                            || (!ROAM.getTargetTank())
                            || (shooterid != ROAM.getTargetTank()->getId())
                            || BZDB.isTrue("enableLocalShotEffect"))
                    {
                        /*EFFECTS.addShotEffect(shooter->getColor(), shotPos,
                                              shooter->getAngle(),
                                              shooter->getVelocity());*/
                    }
                }
            }
            else
                break;
        }
        else
            World::getWorld()->getWorldWeapons()->addShot(firingInfo);

        if (human)
        {
            const float* pos = firingInfo.shot.pos;
            const bool importance = false;
            //const bool localSound = isViewTank(shooter);
            /*if (firingInfo.flagType == Flags::ShockWave)
                playSound(SFX_SHOCK, pos, importance, localSound);
            else if (firingInfo.flagType == Flags::Laser)
                playSound(SFX_LASER, pos, importance, localSound);
            else if (firingInfo.flagType == Flags::GuidedMissile)
                playSound(SFX_MISSILE, pos, importance, localSound);
            else if (firingInfo.flagType == Flags::Thief)
                playSound(SFX_THIEF, pos, importance, localSound);
            else
                playSound(SFX_FIRE, pos, importance, localSound);*/
        }
        break;
    }

    case MsgShotEnd:
    {
        PlayerId id;
        int16_t shotId;
        uint16_t reason;
        msg = nboUnpackUByte(msg, id);
        msg = nboUnpackShort(msg, shotId);
        msg = nboUnpackUShort(msg, reason);
        BaseLocalPlayer* localPlayer = getLocalPlayer(id);

        if (localPlayer)
            localPlayer->endShot(int(shotId), false, reason == 0);
        else
        {
            Player *pl = lookupPlayer(id);
            if (pl)
                pl->endShot(int(shotId), false, reason == 0);
        }
        break;
    }

    case MsgHandicap:
    {
        PlayerId id;
        uint8_t numHandicaps;
        int16_t handicap;
        msg = nboUnpackUByte(msg, numHandicaps);
        for (uint8_t s = 0; s < numHandicaps; s++)
        {
            msg = nboUnpackUByte(msg, id);
            msg = nboUnpackShort(msg, handicap);

            Player *sPlayer = getLocalPlayer(id);
            if (!sPlayer)
            {
                int i = lookupPlayerIndex(id);
                if (i >= 0)
                    sPlayer = remotePlayers[i];
                else
                    logDebugMessage(1, "Received handicap update for unknown player!\n");
            }
            if (sPlayer)
            {
                // a relative score of -_handicapScoreDiff points will provide maximum handicap
                float normalizedHandicap = float(handicap)
                                           / std::max(1.0f, BZDB.eval(StateDatabase::BZDB_HANDICAPSCOREDIFF));

                /* limit how much of a handicap is afforded, and only provide
                 * handicap advantages instead of disadvantages.
                 */
                if (normalizedHandicap > 1.0f)
                    // advantage
                    normalizedHandicap  = 1.0f;
                else if (normalizedHandicap < 0.0f)
                    // disadvantage
                    normalizedHandicap  = 0.0f;

                sPlayer->setHandicap(normalizedHandicap);
            }
        }
        break;
    }

    case MsgScore:
    {
        uint8_t numScores;
        PlayerId id;
        uint16_t wins, losses, tks;
        msg = nboUnpackUByte(msg, numScores);

        for (uint8_t s = 0; s < numScores; s++)
        {
            msg = nboUnpackUByte(msg, id);
            msg = nboUnpackUShort(msg, wins);
            msg = nboUnpackUShort(msg, losses);
            msg = nboUnpackUShort(msg, tks);

            Player *sPlayer = NULL;
            if (id == myTank->getId())
                sPlayer = myTank;
            else
            {
                int i = lookupPlayerIndex(id);
                if (i >= 0)
                    sPlayer = remotePlayers[i];
                else
                    logDebugMessage(1,"Received score update for unknown player!\n");
            }
            if (sPlayer)
                sPlayer->changeScore(wins - sPlayer->getWins(),
                                     losses - sPlayer->getLosses(),
                                     tks - sPlayer->getTeamKills());
        }
        break;
    }

    case MsgSetVar:
    {
        msg = handleMsgSetVars(msg);
        break;
    }

    case MsgTeleport:
    {
        PlayerId id;
        uint16_t from, to;
        msg = nboUnpackUByte(msg, id);
        msg = nboUnpackUShort(msg, from);
        msg = nboUnpackUShort(msg, to);
        Player* tank = lookupPlayer(id);
        if (tank && tank != myTank)
        {
            int face;
            const Teleporter* teleporter = world->getTeleporter(int(to), face);
            if (teleporter)
            {
                const float* pos = teleporter->getPosition();
                tank->setTeleport(TimeKeeper::getTick(), short(from), short(to));
                //playWorldSound(SFX_TELEPORT, pos);
            }
        }
        break;
    }

    case MsgTransferFlag:
    {
        PlayerId fromId, toId;
        unsigned short flagIndex;
        msg = nboUnpackUByte(msg, fromId);
        msg = nboUnpackUByte(msg, toId);
        msg = nboUnpackUShort(msg, flagIndex);
        msg = world->getFlag(int(flagIndex)).unpack(msg);
        Player* fromTank = lookupPlayer(fromId);
        Player* toTank = lookupPlayer(toId);
        handleFlagTransferred( fromTank, toTank, flagIndex);
        break;
    }


    case MsgMessage:
    {
        PlayerId src;
        PlayerId dst;
        uint8_t type;
        msg = nboUnpackUByte(msg, src);
        msg = nboUnpackUByte(msg, dst);
        msg = nboUnpackUByte(msg, type);

        // Only bother processing the message if we know how to handle it
        if (MessageType(type) != ChatMessage && MessageType(type) != ActionMessage)
            break;

        Player* srcPlayer = lookupPlayer(src);
        Player* dstPlayer = lookupPlayer(dst);
        TeamColor dstTeam = PlayerIdToTeam(dst);
        bool toAll = (dst == AllPlayers);
        bool fromServer = (src == ServerPlayer);
        bool toAdmin = (dst == AdminPlayers);
        std::string dstName;

        const std::string srcName = fromServer ? "SERVER" : (srcPlayer ? srcPlayer->getCallSign() : "(UNKNOWN)");
        if (dstPlayer)
            dstName = dstPlayer->getCallSign();
        else if (toAdmin)
            dstName = "Admin";
        else
            dstName = "(UNKNOWN)";
        std::string fullMsg;

        bool ignore = false;
        unsigned int i;
        if (!fromServer)
        {
            for (i = 0; i < silencePlayers.size(); i++)
            {
                const std::string &silenceCallSign = silencePlayers[i];
                if (srcName == silenceCallSign || "*" == silenceCallSign || ("-" == silenceCallSign && srcPlayer
                        && !srcPlayer->isRegistered()))
                {
                    ignore = true;
                    break;
                }
            }
        }

        if (ignore)
        {
#ifdef DEBUG
            // to verify working
            std::string msg2 = "Ignored Msg from " + srcName;
            if (silencePlayers[i] == "*")
                msg2 += " because all chat is ignored";
            else if (silencePlayers[i] == "-")
                msg2 += " because chat from unregistered players is ignored";
            addMessage(NULL,msg2);
#endif
            break;
        }

        std::string origText;
        // if filtering is turned on, filter away the goo
        if (wordfilter != NULL)
        {
            char filtered[MessageLen];
            strncpy(filtered, (const char *)msg, MessageLen - 1);
            filtered[MessageLen - 1] = '\0';
            if (wordfilter->filter(filtered))
                msg = filtered;
            origText = stripAnsiCodes(std::string((const char*)msg));   // use filtered[] while it is in scope
        }
        else
            origText = stripAnsiCodes(std::string((const char*)msg));

        //std::string text = BundleMgr::getCurrentBundle()->getLocalString(origText);
        std::string text = origText;

        if (toAll || toAdmin || srcPlayer == myTank  || dstPlayer == myTank ||
                dstTeam == myTank->getTeam())
        {
            // message is for me
            std::string colorStr;
            if (srcPlayer == NULL)
                colorStr += ColorStrings[RogueTeam];
            else
            {
                const PlayerId pid = srcPlayer->getId();
                if (pid < 200)
                {
                    if (srcPlayer && srcPlayer->getTeam() != NoTeam)
                        colorStr += Team::getAnsiCode(srcPlayer->getTeam());
                    else
                        colorStr += ColorStrings[RogueTeam];
                }
                else if (pid == ServerPlayer)
                    colorStr += ColorStrings[YellowColor];
                else
                {
                    colorStr += ColorStrings[CyanColor]; // replay observers
                }
            }

            fullMsg += colorStr;

            // direct message to or from me
            if (dstPlayer)
            {
                //if (fromServer && (origText == "You are now an administrator!"
                //             || origText == "Password Accepted, welcome back."))
                //admin = true;

                // talking to myself? that's strange
                if (dstPlayer == myTank && srcPlayer == myTank)
                    fullMsg = text;
                else
                {
                    if (BZDB.get("killerhighlight") == "1")
                        fullMsg += ColorStrings[PulsatingColor];
                    else if (BZDB.get("killerhighlight") == "2")
                        fullMsg += ColorStrings[UnderlineColor];

                    if (srcPlayer == myTank)
                    {
                        if (MessageType(type) == ActionMessage)
                            fullMsg += "[->" + dstName + "][" + srcName + " " + text + "]";
                        else
                        {
                            fullMsg += "[->" + dstName + "]";
                            fullMsg += ColorStrings[ResetColor] + " ";
                            fullMsg += ColorStrings[CyanColor] + text;
                        }
                    }
                    else
                    {
                        if (MessageType(type) == ActionMessage)
                            fullMsg += "[" + srcName + " " + text + "]";
                        else
                        {
                            fullMsg += "[" + srcName + "->]";
                            fullMsg += ColorStrings[ResetColor] + " ";
                            fullMsg += ColorStrings[CyanColor] + text;
                        }

                        if (srcPlayer)
                            myTank->setRecipient(srcPlayer);

                        // play a sound on a private message not from self or server

                        bool playSound = !fromServer;
                        if (BZDB.isSet("beepOnServerMsg") && BZDB.isTrue("beepOnServerMsg"))
                            playSound = true;

                        if (playSound)
                        {
                            static TimeKeeper lastMsg = TimeKeeper::getSunGenesisTime();
                            //if (TimeKeeper::getTick() - lastMsg > 2.0f)
                                //playLocalSound( SFX_MESSAGE_PRIVATE );
                            lastMsg = TimeKeeper::getTick();
                        }
                    }
                }
            }
            else
            {
                // team / admin message
                if (toAdmin)
                {

                    // play a sound on a private message not from self or server
                    if (!fromServer)
                    {
                        static TimeKeeper lastMsg = TimeKeeper::getSunGenesisTime();
                        //if (TimeKeeper::getTick() - lastMsg > 2.0f)
                            //playLocalSound( SFX_MESSAGE_ADMIN );
                        lastMsg = TimeKeeper::getTick();
                    }

                    fullMsg += "[Admin] ";
                }

                if (dstTeam != NoTeam)
                {
#ifdef BWSUPPORT
                    fullMsg = "[to ";
                    fullMsg += Team::getName(TeamColor(dstTeam));
                    fullMsg += "] ";
#else
                    fullMsg += "[Team] ";
#endif

                    // play a sound if I didn't send the message
                    if (srcPlayer != myTank)
                    {
                        static TimeKeeper lastMsg = TimeKeeper::getSunGenesisTime();
                        //if (TimeKeeper::getTick() - lastMsg > 2.0f)
                            //playLocalSound(SFX_MESSAGE_TEAM);
                        lastMsg = TimeKeeper::getTick();
                    }
                }

                // display action messages differently
                if (MessageType(type) == ActionMessage)
                    fullMsg += srcName + " " + text;
                else
                    fullMsg += srcName + colorStr + ": " + ColorStrings[CyanColor] + text;
            }

            std::string oldcolor = "";
            if (!srcPlayer || srcPlayer->getTeam() == NoTeam)
                oldcolor = ColorStrings[RogueTeam];
            else if (srcPlayer->getTeam() == ObserverTeam)
                oldcolor = ColorStrings[CyanColor];
            else
                oldcolor = Team::getAnsiCode(srcPlayer->getTeam());
            if (fromServer)
                addMessage(NULL, fullMsg, 2, false, oldcolor.c_str());
            else
                addMessage(NULL, fullMsg, 1, false, oldcolor.c_str());

            //if (!srcPlayer || srcPlayer!=myTank)
                //hud->setAlert(0, fullMsg.c_str(), 3.0f, false);
        }
        break;
    }

    case MsgReplayReset:
    {
        int i;
        unsigned char lastPlayer;
        msg = nboUnpackUByte(msg, lastPlayer);

        // remove players up to 'lastPlayer'
        // any PlayerId above lastPlayer is a replay observers
        for (i=0 ; i<lastPlayer ; i++)
        {
            if (removePlayer (i))
                checkScores = true;
        }

        // remove all of the flags
        for (i=0 ; i<numFlags; i++)
        {
            ::Flag& flag = world->getFlag(i);
            flag.owner = (PlayerId) -1;
            flag.status = FlagNoExist;
            world->initFlag (i);
        }
        break;
    }

    case MsgAdminInfo:
    {
        uint8_t numIPs;
        msg = nboUnpackUByte(msg, numIPs);

        /* if we're getting this, we have playerlist perm */
        myTank->setPlayerList(true);

        // replacement for the normal MsgAddPlayer message
        if (numIPs == 1)
        {
            uint8_t ipsize;
            uint8_t index;
            Address ip;
            const void* tmpMsg = msg; // leave 'msg' pointing at the first entry

            tmpMsg = nboUnpackUByte(tmpMsg, ipsize);
            tmpMsg = nboUnpackUByte(tmpMsg, index);
            tmpMsg = ip.unpack(tmpMsg);
            int playerIndex = lookupPlayerIndex(index);
            Player* tank = getPlayerByIndex(playerIndex);
            if (!tank)
                break;

            std::string message("joining as ");
            if (tank->getTeam() == ObserverTeam)
                message += "an observer";
            else
            {
                switch (tank->getPlayerType())
                {
                case TankPlayer:
                    message += "a tank";
                    break;
                case ComputerPlayer:
                    message += "a robot tank";
                    break;
                default:
                    message += "an unknown type";
                    break;
                }
            }
            message += " from " + ip.getDotNotation();
            tank->setIpAddress(ip);
            addMessage(tank, message);
        }

        // print fancy version to be easily found
        if ((numIPs != 1) || (BZDB.evalInt("showips") > 0))
        {
            uint8_t playerId;
            uint8_t addrlen;
            Address addr;

            for (int i = 0; i < numIPs; i++)
            {
                msg = nboUnpackUByte(msg, addrlen);
                msg = nboUnpackUByte(msg, playerId);
                msg = addr.unpack(msg);

                int playerIndex = lookupPlayerIndex(playerId);
                Player *_player = getPlayerByIndex(playerIndex);
                if (!_player) continue;
                //printIpInfo(_player, addr, "(join)");
                _player->setIpAddress(addr); // save for the signoff message
            } // end for loop
        }
        break;
    }

    case MsgFlagType:
    {
        FlagType* typ = NULL;
        FlagType::unpackCustom(msg, typ);
        logDebugMessage(1, "Got custom flag type from server: %s\n", typ->information().c_str());
        break;
    }

    case MsgPlayerInfo:
    {
        uint8_t numPlayers;
        int i;
        msg = nboUnpackUByte(msg, numPlayers);
        for (i = 0; i < numPlayers; ++i)
        {
            PlayerId id;
            msg = nboUnpackUByte(msg, id);
            Player *p = lookupPlayer(id);
            uint8_t info;
            // parse player info bitfield
            msg = nboUnpackUByte(msg, info);
            if (!p)
                continue;
            p->setAdmin((info & IsAdmin) != 0);
            p->setRegistered((info & IsRegistered) != 0);
            p->setVerified((info & IsVerified) != 0);
        }
        break;
    }

    // inter-player relayed message
    case MsgPlayerUpdate:
    case MsgPlayerUpdateSmall:
    case MsgGMUpdate:
    case MsgLagPing:
        handlePlayerMessage(code, 0, msg);
        break;
    }

    //if (checkScores) updateHighScores();
}

void BZFlagNew::updateFlags(float dt) {
    for (int i = 0; i < numFlags; i++)
    {
        ::Flag& flag = world->getFlag(i);
        if (flag.status == FlagOnTank)
        {
            // position flag on top of tank
            Player* tank = lookupPlayer(flag.owner);
            if (tank)
            {
                const float* pos = tank->getPosition();
                flag.position[0] = pos[0];
                flag.position[1] = pos[1];
                flag.position[2] = pos[2] + tank->getDimensions()[2];
            }
        }
        world->updateFlag(i, dt);
    }
    //FlagSceneNode::waveFlag(dt);
}

bool BZFlagNew::processWorldChunk(const void *buf, uint16_t len, int bytesLeft) {
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

void BZFlagNew::checkEnvironment() {
    if (!myTank) return;

    if (myTank->getTeam() == ObserverTeam )
    {
        if (BZDB.evalInt("showVelocities") <= 2)
            return;

        // Check for an observed tanks hit.
        Player *target = ROAM.getTargetTank();
        const ShotPath* hit = NULL;
        FlagType* flagd;
        float minTime = Infinity;
        int i;

        // Always a possibility of failure
        if (target == NULL)
            return;

        if (ROAM.getMode() != Roaming::roamViewFP)
            // Only works if we are driving with the target
            return;

        if (!target->isAlive() || target->isPaused())
            // If he's dead or paused, don't bother checking
            return;

        flagd = target->getFlag();
        if ((flagd == ::Flags::Narrow) || (flagd == ::Flags::Tiny))
            // Don't bother trying to figure this out with a narrow or tiny flag yet.
            return;

        myTank->checkHit(myTank, hit, minTime);
        for (i = 0; i < curMaxPlayers; i++)
            if (remotePlayers[i])
                myTank->checkHit(remotePlayers[i], hit, minTime);

        if (!hit)
            return;

        Player* hitter = lookupPlayer(hit->getPlayer());
        std::ostringstream smsg;
        if (hitter->getId() == target->getId())
            return;

        // Don't report collisions when teammates can't be killed.
        // This is required because checkHit() tests as if we were observer.
        TeamColor team = hitter->getTeam();
        if (!World::getWorld()->allowTeamKills() && team == target->getTeam() &&
                team != RogueTeam && team != ObserverTeam)
            return;

        smsg << "local collision with "
             << hit->getShotId()
             << " from "
             << hitter->getCallSign()
             << std::endl;
        addMessage(target, smsg.str());

        if (target->hitMap.find(hit->getShotId()) == target->hitMap.end())
            target->computedHits++;

        target->hitMap[hit->getShotId()] = true;
        return;
    }
}

bool BZFlagNew::isKillable(const Player* target) {
    if (target == myTank)
        return false;
    if (target->getTeam() == RogueTeam)
        return true;
    if (myTank->getFlag() == ::Flags::Colorblindness)
        return true;
    if (! World::getWorld()->allowTeamKills() || ! World::getWorld()->allowTeams())
        return true;
    if (target->getTeam() != myTank->getTeam())
        return true;

    return false;
}

void BZFlagNew::doMotion() {
        // Implement based on playing.cxx
}

void BZFlagNew::joinInternetGame()
{
    // check server address
    if (serverNetworkAddress.isAny())
    {
        //HUDDialogStack::get()->setFailedMessage("Server not found");
        //std::cout << "Server not found!" << std::endl;
        console.addLog("Server not found!");
        return;
    }

    // check for a local server block
    ServerAccessList.reload();
    std::vector<std::string> nameAndIp;
    nameAndIp.push_back(startupInfo.serverName);
    nameAndIp.push_back(serverNetworkAddress.getDotNotation());
    if (!ServerAccessList.authorized(nameAndIp))
    {
        //HUDDialogStack::get()->setFailedMessage("Server Access Denied Locally");
        //std::cout << "Server Access Denied Locally" << std::endl;
        console.addLog("Server Access Denied Locally");
        std::string msg = ColorStrings[WhiteColor];
        msg += "NOTE: ";
        msg += ColorStrings[GreyColor];
        msg += "server access is controlled by ";
        msg += ColorStrings[YellowColor];
        msg += ServerAccessList.getFilePath();
        addMessage(NULL, msg);
        console.addLog(msg.c_str());
        return;
    }
    // open server
    _serverLink = new ServerLink(serverNetworkAddress,
            startupInfo.serverPort);
    
    // assume everything's okay for now
    serverDied = false;
    serverError = false;

    if (!_serverLink)
    {
        console.addLog("Server link error");
        //std::cout << "Server link error" << std::endl;
        return;
    }

    if (_serverLink->getState() != ServerLink::Okay)
    {
        switch (_serverLink->getState())
        {
        case ServerLink::BadVersion:
        {
            char versionError[37];
            snprintf(versionError, sizeof(versionError), "Incompatible server version %s", serverLink->getVersion());
            //std::cout << versionError << std::endl;
            //HUDDialogStack::get()->setFailedMessage(versionError);
            console.addLog(versionError);
            break;
        }

        // you got banned
        case ServerLink::Refused:
        {
            const std::string& rejmsg = _serverLink->getRejectionMessage();

            // add to the HUD
            std::string msg;
            msg += "You have been banned from this server";
            //std::cout << msg << std::endl;
            console.addLog(msg.c_str());
            //HUDDialogStack::get()->setFailedMessage(msg.c_str());

            break;
        }

        case ServerLink::Rejected:
            // the server is probably full or the game is over.  if not then
            // the server is having network problems.
            console.addLog("Game is full or over.  Try again later.");
            break;

        case ServerLink::SocketError:
            console.addLog("Error connecting to server.");
            break;

        case ServerLink::CrippledVersion:
            // can't connect to (otherwise compatible) non-crippled server
            console.addLog("Cannot connect to full version server.");
            break;

        default:
            console.addLog("Internal error connecting to server (error code %d).", _serverLink->getState());
            break;
        }
        return;
    }

    // use parallel UDP if desired and using server relay
    if (startupInfo.useUDPconnection)
        _serverLink->sendUDPlinkRequest();
    else
        printError("No UDP connection, see Options to enable.");

    //std::cout << "Connection established!" << std::endl;
    console.addLog("Connection established!");
    sendFlagNegotiation();
    joiningGame = true;
    //scoreboard->huntReset();
    GameTime::reset();
}

void BZFlagNew::sendFlagNegotiation() {
    char msg[MaxPacketLen];
    FlagTypeMap::iterator i;
    char *buf = msg;

    /* Send MsgNegotiateFlags to the server with
     * the abbreviations for all the flags we support.
     */
    for (i = FlagType::getFlagMap().begin();
            i != FlagType::getFlagMap().end();
            ++i)
        buf = (char*) i->second->pack(buf);
    _serverLink->send(MsgNegotiateFlags, buf - msg, msg);
}

void BZFlagNew::leaveGame() {
    entered = false;
    joiningGame = false;

    // Clear the world scene
    worldRenderer.destroyWorldObject();

    // my tank goes away
    const bool sayGoodbye = (myTank != NULL);
    LocalPlayer::setMyTank(NULL);
    delete myTank;
    myTank = NULL;

    // reset the daylight time
    const bool syncTime = (BZDB.eval(StateDatabase::BZDB_SYNCTIME) >= 0.0f);
    const bool fixedTime = BZDB.isSet("fixedTime");
    /*if (syncTime)
    {
        // return to the desired user time
        epochOffset = userTimeEpochOffset;
    }
    else if (fixedTime)
    {
        // save the current user time
        userTimeEpochOffset = epochOffset;
    }
    else
    {
        // revert back to when the client was started?
        epochOffset = userTimeEpochOffset;
    }
    updateDaylight(epochOffset, *sceneRenderer);
    lastEpochOffset = epochOffset;
    BZDB.set(StateDatabase::BZDB_SYNCTIME,
             BZDB.getDefault(StateDatabase::BZDB_SYNCTIME));
    */
    // flush downloaded textures (before the BzMaterials are nuked)
    Downloads::removeTextures();

    // delete world
    World::setWorld(NULL);
    delete world;
    world = NULL;
    teams = NULL;
    curMaxPlayers = 0;
    numFlags = 0;
    remotePlayers = NULL;

    ServerLink::setServer(NULL);
    delete _serverLink;
    serverLink = NULL;
    serverNetworkAddress = Address();

    gameOver = false;
    serverError = false;
    serverDied = false;

    // reset the BZDB variables
    //BZDB.iterate(resetServerVar, NULL);

    ::Flags::clearCustomFlags();
}

int main(int argc, char** argv)
{
    BZFlagNew app({argc, argv});
    return app.main();
}

//MAGNUM_APPLICATION_MAIN(BZFlagNew)