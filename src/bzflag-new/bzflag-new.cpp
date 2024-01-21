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

#include "Magnum/GL/GL.h"
#include "Magnum/SceneGraph/SceneGraph.h"
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

#include <ctime>
#include <cassert>
#include "StartupInfo.h"
#include "StateDatabase.h"
#include "playing.h"
#include "common.h"
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

// defaults for bzdb
#include "defaultBZDB.h"

bool            echoToConsole = false;
bool            echoAnsi = false;
int         debugLevel = 0;
std::string     alternateConfig;
struct tm       userTime;
const char*     argv0;

const bool devDriving = false;

class BasicLoggingCallback : public LoggingCallback {
    public:
        void log(int lvl, const char* msg) override;
};

void BasicLoggingCallback::log(int lvl, const char* msg)
{
    std::cout << lvl << " " << msg << std::endl;
}

BasicLoggingCallback blc;

class WorldDownLoader : cURLManager
{
public:
    void start(char * hexDigest);
private:
    void askToBZFS();
    virtual void finalization(char *data, unsigned int length, bool good);
};

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
        .setLightPositions({{camera.cameraMatrix().transformPoint({-3.0f, 10.0f, 10.0f}), 0.0f}})
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .draw(_mesh);
}

void TexturedDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setLightPositions({{camera.cameraMatrix().transformPoint({-3.0f, 10.0f, 10.0f}), 0.0f}})
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .bindDiffuseTexture(_texture)
        .draw(_mesh);
} 

class BZFlagNew: public Platform::Sdl2Application {
    public:
        explicit BZFlagNew(const Arguments& arguments);
        int main();


    private:
        void startPlaying();
        void playingLoop();

        static void startupErrorCallback(const char* msg);

        void initAres();
        void killAres();

        void joinInternetGame();
        void sendFlagNegotiation();
        void leaveGame();

        void doMotion();

        bool isKillable(const Player* target);

        void drawEvent() override;
        void viewportEvent(ViewportEvent& e) override;
        void mousePressEvent(MouseEvent& e) override;
        void mouseReleaseEvent(MouseEvent& e) override;
        void mouseMoveEvent(MouseMoveEvent& e) override;
        void mouseScrollEvent(MouseScrollEvent& e) override;
        
        Vector3 positionOnSphere(const Vector2i& position) const;

        WorldDownLoader *worldDownLoader;
        double lastObserverUpdateTime = -1;
        StartupInfo startupInfo;
        MessageOfTheDay *motd;
        bool joinRequested = false;
        bool waitingDNS = false;
        Address serverNetworkAddress;
        bool justJoined = false;
        ServerLink *_serverLink;
        bool serverDied = true;
        bool serverError = true;
        bool joiningGame = false;
        bool entered = false;
        LocalPlayer *myTank = NULL;
        Team *teams = NULL;

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
};

void BZFlagNew::initAres()
{
    ares = new AresHandler(0);
}

void BZFlagNew::killAres()
{
    delete ares;
    ares = NULL;
}

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
        .setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.01f, 1000.0f))
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
    if (!importer || !importer->openFile("scene.obj"))
        std::exit(1);

    //Warning{} << args.value("file");

    _textures = Containers::Array<Containers::Optional<GL::Texture2D>>{importer->textureCount()};
    for (UnsignedInt i = 0; i != importer->textureCount(); ++i) {
        Containers::Optional<Trade::TextureData> textureData = importer->texture(i);
        if (!textureData || textureData->type() != Trade::TextureType::Texture2D) {
            Warning{} << "Cannot load texture" << i << importer->textureName(i);
            continue;
        }

        Containers::Optional<Trade::ImageData2D> imageData = importer->image2D(textureData->image());
        if (!imageData || !imageData->isCompressed()) {
            Warning{} << "Cannot load image" << textureData->image() << importer->image2DName(textureData->image());
            continue;
        }

        (*(_textures[i] = GL::Texture2D{}))
            .setMagnificationFilter(textureData->magnificationFilter())
            .setMinificationFilter(textureData->minificationFilter(), textureData->mipmapFilter())
            .setWrapping(textureData->wrapping().xy())
            .setStorage(Math::log2(imageData->size().max()) + 1, GL::textureFormat(imageData->format()), imageData->size())
            .setSubImage(0, {}, *imageData)
            .generateMipmap();
    }
        
    Containers::Array<Containers::Optional<Trade::PhongMaterialData>> materials{importer->materialCount()};
    for (UnsignedInt i = 0; i != importer->materialCount(); ++i) {
        Containers::Optional<Trade::MaterialData> materialData;
        if (!(materialData = importer->material(i))) {
            Warning{} << "Cannot load material" << i << importer->materialName(i);
            continue;
        }
        materials[i] = std::move(*materialData).as<Trade::PhongMaterialData>();
    }

    _meshes = Containers::Array<Containers::Optional<GL::Mesh>>{importer->meshCount()};
    for (UnsignedInt i = 0; i != importer->meshCount(); ++i) {
        Containers::Optional<Trade::MeshData> meshData;
        if (!(meshData = importer->mesh(i))) {
            Warning{} << "Cannot load mesh" << i << importer->meshName(i);
            continue;
        }
        MeshTools::CompileFlags flags;
        if (meshData->hasAttribute(Trade::MeshAttribute::Normal))
            flags |= MeshTools::CompileFlag::GenerateFlatNormals;
        _meshes[i] = MeshTools::compile(*meshData, flags);
    }

    Warning{} << "Mesh count:" << importer->meshCount();

    // Edge case where the file doesn't have any scene (just a mesh)
    if (importer->defaultScene() == -1) {
        if (!_meshes.isEmpty() && _meshes[0])
            new ColoredDrawable{_manipulator, _coloredShader, *_meshes[0], 0xffffff_rgbf, _drawables};
        return;
    }

    Containers::Optional<Trade::SceneData> scene;
    if (!(scene = importer->scene(importer->defaultScene())) ||
        !scene->is3D() || !scene->hasField(Trade::SceneField::Parent) || !scene->hasField(Trade::SceneField::Mesh)) {
        Fatal{} << "Cannot load scene" << importer->defaultScene() << importer->sceneName(importer->defaultScene());
        }
    Containers::Array<Object3D*> objects{std::size_t(scene->mappingBound())};
    Containers::Array<Containers::Pair<UnsignedInt, Int>> parents = scene->parentsAsArray();
    for (const Containers::Pair<UnsignedInt, Int>& parent: parents) {
        objects[parent.first()] = new Object3D{};
    }
    for (const Containers::Pair<UnsignedInt, Int>& parent: parents) {
        objects[parent.first()]->setParent(parent.second() == -1 ? &_manipulator : objects[parent.second()]);
    }

    for (const Containers::Pair<UnsignedInt, Containers::Pair<UnsignedInt, Int>>& meshMaterial: scene->meshesMaterialsAsArray()) {
        Object3D* object = objects[meshMaterial.first()];
        Containers::Optional<GL::Mesh>& mesh = _meshes[meshMaterial.second().first()];
        
        if (!object || !mesh) continue;

        Int materialId = meshMaterial.second().second();

        if (materialId == -1 || !materials[materialId]) {
            new ColoredDrawable{*object, _coloredShader, *mesh, 0xfffffff_rgbf, _drawables};
        } else if (materials[materialId]->hasAttribute(Trade::MaterialAttribute::DiffuseTexture) && _textures[materials[materialId]->diffuseTexture()]) {
            new TexturedDrawable{*object, _texturedShader, *mesh, *_textures[materials[materialId]->diffuseTexture()], _drawables};
        } else {
            new ColoredDrawable{*object, _coloredShader, *mesh, materials[materialId]->diffuseColor(), _drawables};
        }
    }
        
}

void BZFlagNew::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    _camera->draw(_drawables);

    swapBuffers();
}

void BZFlagNew::viewportEvent(ViewportEvent& e) {
    GL::defaultFramebuffer.setViewport({{}, e.framebufferSize()});
    _camera->setViewport(e.windowSize());
}

void BZFlagNew::mousePressEvent(MouseEvent& e) {
    if (e.button() == MouseEvent::Button::Left)
        _previousPosition = positionOnSphere(e.position());
}

void BZFlagNew::mouseReleaseEvent(MouseEvent& e) {
    if (e.button() == MouseEvent::Button::Left)
        _previousPosition = Vector3{};
}

void BZFlagNew::mouseScrollEvent(MouseScrollEvent& e) {
    if (!e.offset().y()) return;
    const Float distance = _cameraObject.transformation().translation().z();
    _cameraObject.translate(Vector3::zAxis(distance*(1.0f - (e.offset().y() > 0 ? 1/0.85f : 0.85f))));
    redraw();
}

void BZFlagNew::mouseMoveEvent(MouseMoveEvent &e) {
    if (!(e.buttons() & MouseMoveEvent::Button::Left)) return;

    const Vector3 currentPosition = positionOnSphere(e.position());
    const Vector3 axis = Math::cross(_previousPosition, currentPosition);

    if (_previousPosition.length() < 0.001f || axis.length() < 0.001f) return;

    _manipulator.rotate(Math::angle(_previousPosition, currentPosition), axis.normalized());
    _previousPosition = currentPosition;

    redraw();
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

    //initConfigData();
    loadBZDBDefaults();

    ServerListCache::get()->loadCache();

    BZDB.set("callsign", "bzflagdevtest");
    BZDB.set("server", "localhost");
    BZDB.set("port", "5154");

    startupInfo.useUDPconnection=true;
    startupInfo.team = ObserverTeam;
    strcpy(startupInfo.callsign, "testingbz");
    strcpy(startupInfo.serverName, "localhost");
    startupInfo.serverPort = 5154;

    startupInfo.autoConnect = true;

    Team::updateShotColors();

    startPlaying();

    killAres();
    AresHandler::globalShutdown();
    return 0;
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

    worldDownLoader = new WorldDownLoader;

    playingLoop();

    delete worldDownLoader;

    // leaveGame();
    World::done();
}

void BZFlagNew::playingLoop()
{
        // main loop
    while (!CommandsStandard::isQuit())
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
                    serverList->checkEchos(getStartupInfo());
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
                std::cout << "Ares Handler failed, server not found." << std::endl;
                waitingDNS = false;
            }
            else if (status == AresHandler::HbNSucceeded)
            {
                // now try connecting
                serverNetworkAddress = Address(inAddress);
                char buf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &inAddress.s_addr, buf, sizeof(buf));
                std::cout << "server addr: " << buf << std::endl;
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
                (serverLink && serverLink->getState() == ServerLink::Hungup))
        {
            // if we haven't reported the death yet then do so now
            if (serverDied ||
                    (serverLink && serverLink->getState() == ServerLink::Hungup))
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
                int mx, my;
                mainWindow->getMousePosition(mx, my);
            }
            myTank->update();
        }
        cURLManager::perform();
        mainLoopIteration();
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
        float rotation = 0.0f, speed = 1.0f;
    const int noMotionSize = hud->getNoMotionSize();
    const int maxMotionSize = hud->getMaxMotionSize();

    int keyboardRotation = myTank->getRotation();
    int keyboardSpeed    = myTank->getSpeed();
    /* see if controls are reversed */
    if (myTank->getFlag() == ::Flags::ReverseControls)
    {
        keyboardRotation = -keyboardRotation;
        keyboardSpeed    = -keyboardSpeed;
    }

    // mouse is default steering method; query mouse pos always, not doing so
    // can lead to stuttering movement with X and software rendering (uncertain why)
    int mx = 0, my = 0;
    mainWindow->getMousePosition(mx, my);

    // determine if joystick motion should be used instead of mouse motion
    // when the player bumps the mouse, LocalPlayer::getInputMethod return Mouse;
    // make it return Joystick when the user bumps the joystick
    if (mainWindow->haveJoystick())
    {
        if (myTank->getInputMethod() == LocalPlayer::Joystick)
        {
            // if we're using the joystick right now, replace mouse coords with joystick coords
            mainWindow->getJoyPosition(mx, my);
        }
        else
        {
            // if the joystick is not active, and we're not forced to some other input method,
            // see if it's moved and autoswitch
            if (BZDB.isTrue("allowInputChange"))
            {
                int jx = 0, jy = 0;
                mainWindow->getJoyPosition(jx, jy);
                // if we aren't using the joystick, but it's moving, start using it
                if ((jx < -noMotionSize * 2) || (jx > noMotionSize * 2)
                        || (jy < -noMotionSize * 2) || (jy > noMotionSize * 2))
                    myTank->setInputMethod(LocalPlayer::Joystick); // joystick motion
            } // allowInputChange
        } // getInputMethod == Joystick
    } // mainWindow->Joystick

    /* see if controls are reversed */
    if (myTank->getFlag() == ::Flags::ReverseControls)
    {
        mx = -mx;
        my = -my;
    }

#if defined(FREEZING)
    if (motionFreeze) return;
#endif

    /*if (myTank->isAutoPilot())
        //doAutoPilot(rotation, speed);
    else */if (myTank->getInputMethod() == LocalPlayer::Keyboard)
    {

        rotation = (float)keyboardRotation;
        speed    = (float)keyboardSpeed;
        if (speed < 0.0f)
            speed /= 2.0;

        rotation *= BZDB.eval("displayFOV") / 60.0f;
        if (BZDB.isTrue("slowKeyboard"))
        {
            rotation *= 0.5f;
            speed *= 0.5f;
        }
    }
    else     // both mouse and joystick
    {

        // calculate desired rotation
        if (keyboardRotation && !devDriving)
        {
            rotation = float(keyboardRotation);
            rotation *= BZDB.eval("displayFOV") / 60.0f;
            if (BZDB.isTrue("slowKeyboard"))
                rotation *= 0.5f;
        }
        else if (mx < -noMotionSize)
        {
            rotation = float(-mx - noMotionSize) / float(maxMotionSize - noMotionSize);
            if (rotation > 1.0f)
                rotation = 1.0f;
        }
        else if (mx > noMotionSize)
        {
            rotation = -float(mx - noMotionSize) / float(maxMotionSize - noMotionSize);
            if (rotation < -1.0f)
                rotation = -1.0f;
        }

        // calculate desired speed
        if (keyboardSpeed && !devDriving)
        {
            speed = float(keyboardSpeed);
            if (speed < 0.0f)
                speed *= 0.5f;
            if (BZDB.isTrue("slowKeyboard"))
                speed *= 0.5f;
        }
        else if (my < -noMotionSize)
        {
            speed = float(-my - noMotionSize) / float(maxMotionSize - noMotionSize);
            if (speed > 1.0f)
                speed = 1.0f;
        }
        else if (my > noMotionSize)
        {
            speed = -float(my - noMotionSize) / float(maxMotionSize - noMotionSize);
            if (speed < -0.5f)
                speed = -0.5f;
        }
        else
            speed = 0.0f;
    }

    myTank->setDesiredAngVel(rotation);
    myTank->setDesiredSpeed(speed);
}

void BZFlagNew::joinInternetGame()
{
    // check server address
    if (serverNetworkAddress.isAny())
    {
        //HUDDialogStack::get()->setFailedMessage("Server not found");
        std::cout << "Server not found!" << std::endl;
        return;
    }

    // check for a local server block
    ServerAccessList.reload();
    std::vector<std::string> nameAndIp;
    nameAndIp.push_back(startupInfo.serverName);
    nameAndIp.push_back(serverNetworkAddress.getDotNotation());
    if (!ServerAccessList.authorized(nameAndIp))
    {
        std::string msg;
        //HUDDialogStack::get()->setFailedMessage("Server Access Denied Locally");
        std::cout << "Server Access Denied Locally" << std::endl;
        //std::string msg = ColorStrings[WhiteColor];
        msg += "NOTE: ";
        //msg += ColorStrings[GreyColor];
        msg += "server access is controlled by ";
        //msg += ColorStrings[YellowColor];
        msg += ServerAccessList.getFilePath();
        //addMessage(NULL, msg);
        std::cout << msg;
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
        std::cout << "Server link error" << std::endl;
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
            std::cout << versionError << std::endl;
            //HUDDialogStack::get()->setFailedMessage(versionError);
            break;
        }

        // you got banned
        case ServerLink::Refused:
        {
            const std::string& rejmsg = _serverLink->getRejectionMessage();

            // add to the HUD
            std::string msg;
            msg += "You have been banned from this server";
            std::cout << msg << std::endl;
            //HUDDialogStack::get()->setFailedMessage(msg.c_str());

            break;
        }

        case ServerLink::Rejected:
            // the server is probably full or the game is over.  if not then
            // the server is having network problems.
            std::cout << "Game is full or over.  Try again later." << std::endl;
            break;

        case ServerLink::SocketError:
            std::cout << "Error connecting to server." << std::endl;
            break;

        case ServerLink::CrippledVersion:
            // can't connect to (otherwise compatible) non-crippled server
            std::cout << "Cannot connect to full version server." << std::endl;
            break;

        default:
            std::cout << "Internal error connecting to server (error code %d)." << std::endl;
            break;
        }
        return;
    }

    // use parallel UDP if desired and using server relay
    if (startupInfo.useUDPconnection)
        _serverLink->sendUDPlinkRequest();
    else
        printError("No UDP connection, see Options to enable.");

    std::cout << "Connection established!" << std::endl;
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
    //Downloads::removeTextures();

    // delete world
    World::setWorld(NULL);
    delete world;
    world = NULL;
    teams = NULL;
    curMaxPlayers = 0;
    numFlags = 0;
    remotePlayers = NULL;
}

int main(int argc, char** argv)
{
    BZFlagNew app({argc, argv});
    return app.main();
}

//MAGNUM_APPLICATION_MAIN(BZFlagNew)