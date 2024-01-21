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
    motd(NULL)
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
            //if (myTank) leaveGame();

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
                joinInternetGame();
                waitingDNS = false;
            }
        }
        cURLManager::perform();
        mainLoopIteration();
    }

}

int main(int argc, char** argv)
{
    BZFlagNew app({argc, argv});
    return app.main();
}

//MAGNUM_APPLICATION_MAIN(BZFlagNew)