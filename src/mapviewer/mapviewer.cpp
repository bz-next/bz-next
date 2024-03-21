#include <Magnum/GL/Context.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/DebugStl.h>
#include <Corrade/Utility/Resource.h>
#include <Corrade/Containers/StridedArrayView.h>

#include <Magnum/GL/Version.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/PixelFormat.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/PixelFormat.h>

#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Types.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Math/FunctionsBatch.h>

#include <Magnum/Image.h>

#include <imgui.h>

#include "DrawableGroupBrowser.h"
#include "GLInfo.h"

#include "Magnum/ImGuiIntegration/Widgets.h"
#include "MagnumBZMaterial.h"
#include "MagnumDefs.h"
#include "MagnumSceneManager.h"
#include "MagnumTextureManager.h"
#include "WorldSceneObjectGenerator.h"

#include "BZTextureBrowser.h"
#include "BZMaterialBrowser.h"
#include "BZMaterialViewer.h"
#include "ObstacleBrowser.h"
#include "BZWTextEditor.h"
#include "AboutDialog.h"
#include "CacheBrowser.h"
#include "TexMatBrowser.h"
#include "DynColorBrowser.h"
#include "MeshTransformBrowser.h"
#include "PhyDrvBrowser.h"
#include "OnlineMapBrowser.h"

#include "WorldMeshGenerator.h"
#include "cURLManager.h"

#include "SceneObjectManager.h"
#include "DrawableGroupManager.h"

#include "MagnumSceneManager.h"
#include "MagnumSceneRenderer.h"

#ifdef TARGET_EMSCRIPTEN
#include "DepthReinterpretShader.h"
#endif

#include <ctime>
#include <cassert>
#include <cstring>
#include <fstream>

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
#include "TimeKeeper.h"

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

class MapViewer: public Platform::Sdl2Application {
    public:
        explicit MapViewer(const Arguments& arguments);
        void init();


    private:
        void resetBZDB();

        void tickEvent() override;

        void drawEvent() override;

        Float depthAt(const Vector2i& windowPosition);
        Vector3 unproject(const Vector2i& windowPosition, Float depth) const;

        void viewportEvent(ViewportEvent& e) override;
        void mousePressEvent(MouseEvent& e) override;
        void mouseReleaseEvent(MouseEvent& e) override;
        void mouseMoveEvent(MouseMoveEvent& e) override;
        void mouseScrollEvent(MouseScrollEvent& e) override;
        void textInputEvent(TextInputEvent& event) override;
        void exitEvent(ExitEvent& event) override;

        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;

        void resetCamera();

        // IMGUI
        void showMainMenuBar();
        void showMenuView();
        void showMenuDebug();

        void drawWindows();

        // Window should draw
        bool showProfiler = false;
        bool showTMBrowser = false;
        bool showMATBrowser = false;
        bool showMATViewer = false;
        bool showObsBrowser = false;
        bool showGLInfo = false;
        bool showEditor = false;
        bool showAbout = false;
        bool showCacheBrowser = false;
        bool showTexMatBrowser = false;
        bool showPhyDrvBrowser = false;
        bool showDynColorBrowser = false;
        bool showMeshTransformBrowser = false;
        bool showMapBrowser = true;
        bool showDrawableGroupBrowser = false;
        bool showRendererSettings = false;
        bool showAdjustSun = false;
        bool showPipelineTexBrowser = false;

        bool showGrid = false;

#ifndef TARGET_EMSCRIPTEN
        void maybeShowFileBrowser();
#endif
        

        static void startupErrorCallback(const char* msg);

#ifdef TARGET_EMSCRIPTEN
        static void handleUploadedMap(const std::string& filename, const std::string& mime_type, std::string_view buffer, void*);
#endif
        static void handleSaveFromEditor(const std::string& filename, const std::string& data);
        static void handleUpdateFromEditor(const std::string& filename, const std::string& data);

        static void handleLoadFromMapBrowser(const CachedResource& mapRsc);

        void loadMap(std::string path, const std::string& data, bool reloadEditor = true);

        void loopIteration();

        Object3D *_sceneRoot;
        Object3D *_cameraObject;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;

        Float _lastDepth;
        Vector2i _lastPosition{-1};
        Vector3 _rotationPoint, _translationPoint;

#ifdef TARGET_EMSCRIPTEN
        GL::Framebuffer _depthFramebuffer{NoCreate};
        GL::Texture2D _depth{NoCreate};

        GL::Framebuffer _reinterpretFramebuffer{NoCreate};
        GL::Renderbuffer _reinterpretDepth{NoCreate};

        GL::Mesh _fullscreenTriangle{NoCreate};
        DepthReinterpretShader _reinterpretShader{NoCreate};
#endif

        WorldSceneObjectGenerator worldSceneObjGen;
        WorldMeshGenerator worldSceneBuilder;
        ImGuiIntegration::Context _imgui{NoCreate};

        MagnumSceneRenderer sceneRenderer;

        BZTextureBrowser tmBrowser;
        BZMaterialBrowser matBrowser;
        BZMaterialViewer matViewer;
        ObstacleBrowser obsBrowser;
        BZWTextEditor editor;
        AboutDialog about;
        CacheBrowser cacheBrowser;
        TexMatBrowser texMatBrowser;
        DynColorBrowser dynColorBrowser;
        MeshTransformBrowser meshTFBrowser;
        PhyDrvBrowser phyDrvBrowser;
        OnlineMapBrowser mapBrowser;
        DrawableGroupBrowser dgrpBrowser;
#ifndef TARGET_EMSCRIPTEN
        ImGui::FileBrowser fileBrowser;
#endif

        // For C-style callbacks from other code / emscripten
        static MapViewer *instance;
};

MapViewer *MapViewer::instance = NULL;

MapViewer::MapViewer(const Arguments& arguments):
    Platform::Sdl2Application{arguments, Configuration{}
        .setTitle("BZFlag Map Viewer")
        .setWindowFlags(Configuration::WindowFlag::Resizable),
        GLConfiguration{}
#if defined(MAGNUM_TARGET_GLES)
        // No multisampling for GLES, assume less capable machine
        .setVersion(GL::Version::GLES300)
#endif
#ifdef TARGET_EMSCRIPTEN
        /* Needed to ensure the canvas depth buffer is always Depth24Stencil8,
           stencil size is 0 by default, some browser enable stencil for that
           (Chrome) and some don't (Firefox) and thus our texture format for
           blitting might not always match.
           WebGL is insane...*/
        .setDepthBufferSize(24)
        .setStencilBufferSize(8)
#endif
        .setSampleCount(4)
        }
{
    using namespace Math::Literals;

    assert(instance == NULL);

    instance = this;

    loggingCallback = &blc;
    debugLevel = 0;

    Utility::Arguments args;
    args
        .setGlobalHelp("BZFlag Map Viewer")
        .parse(arguments.argc, arguments.argv);
    
    /* Try 8x MSAA, fall back to zero samples if not possible. Enable only 2x
       MSAA if we have enough DPI. */
    /*{
        const Vector2 dpiScaling = this->dpiScaling({});
        Configuration conf;
        conf.setTitle("BZFlag Map Viewer")
            .setWindowFlags(Configuration::WindowFlag::Resizable)
            .setSize(conf.size(), dpiScaling);
        GLConfiguration glConf;
        glConf.setSampleCount(dpiScaling.max() < 2.0f ? 8 : 2);
#ifdef TARGET_EMSCRIPTEN
        glConf.setDepthBufferSize(24)
            .setStencilBufferSize(8);
#endif
        if(!tryCreate(conf, glConf))
            create(conf, glConf.setSampleCount(0));
    }*/
    
    MagnumSceneManager::initScene();
    sceneRenderer.init();

    _sceneRoot = SOMGR.getObj("Scene");

    _cameraObject = new Object3D{};
    
    (*_cameraObject)
        .setParent(_sceneRoot)
        .translate(Vector3::zAxis(20.0f));
        
    
    (*(_camera = new SceneGraph::Camera3D{*_cameraObject}))
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 1.0f, 1000.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());

    sceneRenderer.resizeViewport(GL::defaultFramebuffer.viewport().size().x(), GL::defaultFramebuffer.viewport().size().y());

    _lastDepth = ((_camera->projectionMatrix()*_camera->cameraMatrix()).transformPoint({}).z() + 1.0f)*0.5f;


    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);

    // Important for proper rendering of skydome
    GL::Renderer::setDepthFunction(GL::Renderer::DepthFunction::LessOrEqual);

    _imgui = ImGuiIntegration::Context(Vector2{windowSize()}/dpiScaling(), windowSize(), framebufferSize());
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

#ifndef TARGET_EMSCRIPTEN
    fileBrowser.SetTitle("Select Map File");
    fileBrowser.SetTypeFilters({".bzw"});
#endif

    editor.setReloadCallback(handleUpdateFromEditor);
    editor.setSaveCallback(handleSaveFromEditor);

    OnlineMapBrowser::setOnDownloadCompleteCallback(handleLoadFromMapBrowser);

    /* Set up proper blending to be used by ImGui. There's a great chance
       you'll need this exact behavior for the rest of your scene. If not, set
       this only for the drawFrame() call. */
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
        GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

#ifdef TARGET_EMSCRIPTEN
    _depth = GL::Texture2D{};
    _depth.setMinificationFilter(GL::SamplerFilter::Nearest)
        .setMagnificationFilter(GL::SamplerFilter::Nearest)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        /* The format is set to combined depth/stencil in hope it will match
           the browser depth/stencil format, requested in the GLConfiguration
           above. If it won't, the blit() won't work properly. */
        .setStorage(1, GL::TextureFormat::Depth24Stencil8, framebufferSize());
    _depthFramebuffer = GL::Framebuffer{{{}, framebufferSize()}};
    _depthFramebuffer.attachTexture(GL::Framebuffer::BufferAttachment::Depth, _depth, 0);

    _reinterpretDepth = GL::Renderbuffer{};
    _reinterpretDepth.setStorage(GL::RenderbufferFormat::RGBA8, framebufferSize());
    _reinterpretFramebuffer = GL::Framebuffer{{{}, framebufferSize()}};
    _reinterpretFramebuffer.attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, _reinterpretDepth);
    _reinterpretShader = DepthReinterpretShader{};
    _fullscreenTriangle = GL::Mesh{};
    _fullscreenTriangle.setCount(3);
#endif

    init();
}

void MapViewer::resetCamera() {
    (*_cameraObject)
        .resetTransformation()
        .translate(Vector3::zAxis(20.0f));
}

void MapViewer::handleSaveFromEditor(const std::string& filename, const std::string& data) {
#ifdef TARGET_EMSCRIPTEN
    emscripten_browser_file::download(filename, "text/plain", data);
#else
    std::ofstream of(filename);
    of << data;
    of.close();
#endif
}

void MapViewer::handleUpdateFromEditor(const std::string& filename, const std::string& data) {
    instance->loadMap(filename, data, false);
}

void MapViewer::handleLoadFromMapBrowser(const CachedResource& mapRsc) {
    std::string data = std::string(mapRsc.getData().begin(), mapRsc.getData().end());
    instance->loadMap(mapRsc.getFilename(), data);
}

void MapViewer::exitEvent(ExitEvent& event) {
    event.setAccepted();
    MagnumTextureManager::instance().clear();
}

#ifdef TARGET_EMSCRIPTEN
void MapViewer::handleUploadedMap(const std::string& filename, const std::string& mime_type, std::string_view buffer, void*) {
    instance->loadMap(filename, std::string(buffer));
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
        if (ImGui::BeginMenu("Scene")) {
            if (ImGui::MenuItem("Renderer Settings", NULL, &showRendererSettings)) {}
            if (ImGui::MenuItem("Adjust Sun", NULL, &showAdjustSun)) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            showMenuDebug();
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("About")) {
            showAbout = true;
        }
        ImGui::EndMainMenuBar();
    }
}

void MapViewer::showMenuView() {
    if (ImGui::MenuItem("Map Browser", NULL, &showMapBrowser)) {}
    ImGui::Separator();
    if (ImGui::MenuItem("Map Editor", NULL, &showEditor)) {}
    ImGui::Separator();
    if (ImGui::MenuItem("Obstacle Browser", NULL, &showObsBrowser)) {}
    if (ImGui::MenuItem("Texture Browser", NULL, &showTMBrowser)) {}
    if (ImGui::MenuItem("Material Browser", NULL, &showMATBrowser)) {}
    if (ImGui::MenuItem("Texture Matrix Browser", NULL, &showTexMatBrowser)) {}
    if (ImGui::MenuItem("Dynamic Color Browser", NULL, &showDynColorBrowser)) {}
    if (ImGui::MenuItem("Physics Driver Browser", NULL, &showPhyDrvBrowser)) {}
    if (ImGui::MenuItem("Mesh Transform Browser", NULL, &showMeshTransformBrowser)) {}
    ImGui::Separator();
    if (ImGui::MenuItem("Material Viewer", NULL, &showMATViewer)) {}
    ImGui::Separator();
    if (ImGui::MenuItem("Cache Browser", NULL, &showCacheBrowser)) {}
    ImGui::Separator();
    if (ImGui::MenuItem("Drawable Group Browser", NULL, &showDrawableGroupBrowser)) {}
#ifndef MAGNUM_TARGET_GLES2
    ImGui::Separator();
    if (ImGui::MenuItem("Grid", NULL, &showGrid)) {}
#endif
}

void MapViewer::showMenuDebug() {
    if (ImGui::MenuItem("Profiler", NULL, &showProfiler)) {}
    if (ImGui::MenuItem("GL Info", NULL, &showGLInfo)) {}
    ImGui::Separator();
    if (ImGui::MenuItem("Recompile World Mesh")) {
        worldSceneObjGen.destroyWorldObject();
        worldSceneObjGen.createWorldObject(&worldSceneBuilder);
    }
    if (ImGui::MenuItem("Force Load Material Textures")) {
        MAGNUMMATERIALMGR.forceLoadTextures();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Pipeline Texture Browser", NULL, &showPipelineTexBrowser)) {}
}

void MapViewer::drawWindows() {
    if (showProfiler) {
        ImGui::Begin("Profiler", &showProfiler);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
            1000.0/Double(ImGui::GetIO().Framerate), Double(ImGui::GetIO().Framerate));
        ImGui::End();
    }

    if (showTMBrowser) {
        tmBrowser.draw("Texture Browser", &showTMBrowser);
    }

    if (showMATBrowser) {
        matBrowser.draw("Material Browser", &showMATBrowser);
    }

    if (showMATViewer) {
        matViewer.draw("Material Viewer", &showMATViewer);
    }

    if (showObsBrowser) {
        obsBrowser.draw("Obstacle Browser", &showObsBrowser);
    }

    static std::string info = getGLInfo();
    if (showGLInfo) {
        ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
        ImGui::Begin("OpenGL Info", &showGLInfo);
        ImGui::TextWrapped(info.c_str());
        ImGui::End();
    }

    if (showEditor) {
        editor.draw("Map Editor", &showEditor);
    }

#ifndef TARGET_EMSCRIPTEN
    // Filebrowser tracks whether to draw internally
    fileBrowser.Display();
    if (fileBrowser.HasSelected()) {
        loadMap(fileBrowser.GetSelected().string(), "", true);
        fileBrowser.ClearSelected();
    }
#endif

    if (showAbout) {
        about.draw("About", &showAbout);
    }

    if (showCacheBrowser) {
        cacheBrowser.draw("Cache Browser", &showCacheBrowser);
    }

    if (showTexMatBrowser) {
        texMatBrowser.draw("Texture Matrix Browser", &showTexMatBrowser);
    }

    if (showDynColorBrowser) {
        dynColorBrowser.draw("Dynamic Color Browser", &showDynColorBrowser);
    }

    if (showMeshTransformBrowser) {
        meshTFBrowser.draw("Mesh Transform Browser", &showMeshTransformBrowser);
    }

    if (showPhyDrvBrowser) {
        phyDrvBrowser.draw("Physics Driver Browser", &showPhyDrvBrowser);
    }

    if (showMapBrowser) {
        mapBrowser.draw("Map Browser", &showMapBrowser);
    }

    if (showDrawableGroupBrowser) {
        dgrpBrowser.draw("Drawable Group Browser", &showDrawableGroupBrowser);
    }

    if (showPipelineTexBrowser) {
        sceneRenderer.drawPipelineTexBrowser("Pipeline Texture Browser", &showPipelineTexBrowser);
    }

    if (showAdjustSun) {
        ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
        ImGui::Begin("Adjust Sun", &showAdjustSun);
        auto pos = sceneRenderer.getSunPosition();
        static float x = pos.x(), y = pos.y(), z = pos.z();
        ImGui::DragFloat("Sun X", &x, 10.0f, -10000.0f, 10000.0f, "%.2f");
        ImGui::DragFloat("Sun Y", &y, 10.0f, -10000.0f, 10000.0f, "%.2f");
        ImGui::DragFloat("Sun Z", &z, 10.0f, 1000.0f, 10000.0f, "%.2f");
        sceneRenderer.setSunPosition({x, y, z});
        ImGui::End();
    }

    if (showRendererSettings) {
        sceneRenderer.drawSettings("Renderer Settings", &showRendererSettings);
    }
}

void MapViewer::drawEvent() {
#ifdef TARGET_EMSCRIPTEN /* Another FB could be bound from the depth read */
    GL::defaultFramebuffer.bind();
#endif
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    _imgui.newFrame();

    showMainMenuBar();
    drawWindows();

    _imgui.updateApplicationCursor(*this);

    if(ImGui::GetIO().WantTextInput && !isTextInputActive())
        startTextInput();
    else if(!ImGui::GetIO().WantTextInput && isTextInputActive())
        stopTextInput();

    sceneRenderer.renderScene(_camera);

    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

    _imgui.drawFrame();

#ifdef TARGET_EMSCRIPTEN
    /* The rendered depth buffer might get lost later, so resolve it to our
       depth texture before swapping it to the canvas */
    GL::Framebuffer::blit(GL::defaultFramebuffer, _depthFramebuffer, GL::defaultFramebuffer.viewport(), GL::FramebufferBlit::Depth);
#endif

    swapBuffers();
}


void MapViewer::tickEvent() {
    loopIteration();
    redraw();
}

Float MapViewer::depthAt(const Vector2i& windowPosition) {
    /* First scale the position from being relative to window size to being
       relative to framebuffer size as those two can be different on HiDPI
       systems */
    const Vector2i position = windowPosition*Vector2{framebufferSize()}/Vector2{windowSize()};
    const Vector2i fbPosition{position.x(), GL::defaultFramebuffer.viewport().sizeY() - position.y() - 1};
    const Range2Di area = Range2Di::fromSize(fbPosition, Vector2i{1}).padded(Vector2i{2});

#ifndef TARGET_EMSCRIPTEN
    GL::defaultFramebuffer.mapForRead(GL::DefaultFramebuffer::ReadAttachment::Front);
    Image2D data = GL::defaultFramebuffer.read(
        Range2Di::fromSize(fbPosition, Vector2i{1}).padded(Vector2i{2}),
        {GL::PixelFormat::DepthComponent, GL::PixelType::Float});

    /* TODO: change to just Math::min<Float>(data.pixels<Float>() when the
       batch functions in Math can handle 2D views */
    return Math::min<Float>(data.pixels<Float>().asContiguous());
#else
    _reinterpretFramebuffer.clearColor(0, Vector4{})
        .bind();
    _reinterpretShader.bindDepthTexture(_depth);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::setScissor(area);
    _reinterpretShader.draw(_fullscreenTriangle);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);

    Image2D image = _reinterpretFramebuffer.read({area}, {PixelFormat::RGBA8Unorm});

    /* Unpack the values back. Can't just use UnsignedInt as the values are
       packed as big-endian. */
    Float depth[25];
    /* TODO: remove the asContiguous() when bit unpack functions exist */
    auto packed = image.pixels<const Math::Vector4<UnsignedByte>>().asContiguous();
    for(std::size_t i = 0; i != packed.size(); ++i) {
        depth[i] = Math::unpack<Float, UnsignedInt, 24>((packed[i].x() << 16) | (packed[i].y() << 8) | packed[i].z());
    }
    return Math::min<Float>(depth);
#endif
}

Vector3 MapViewer::unproject(const Vector2i& windowPosition, Float depth) const {
    /* We have to take window size, not framebuffer size, since the position is
       in window coordinates and the two can be different on HiDPI systems */
    const Vector2i viewSize = windowSize();
    const Vector2i viewPosition{windowPosition.x(), viewSize.y() - windowPosition.y() - 1};
    const Vector3 in{2*Vector2{viewPosition}/Vector2{viewSize} - Vector2{1.0f}, depth*2.0f - 1.0f};

    /*
        Use the following to get global coordinates instead of camera-relative:

        (_cameraObject->absoluteTransformationMatrix()*_camera->projectionMatrix().inverted()).transformPoint(in)
    */
    return _camera->projectionMatrix().inverted().transformPoint(in);
}

void MapViewer::viewportEvent(ViewportEvent& e) {
    GL::defaultFramebuffer.setViewport({{}, e.framebufferSize()});
    _imgui.relayout(Vector2{e.windowSize()}/e.dpiScaling(),
        e.windowSize(), e.framebufferSize());
    _camera->setViewport(e.windowSize());
    /* Recreate textures and renderbuffers that depend on viewport size */
#ifdef TARGET_EMSCRIPTEN
    _depth = GL::Texture2D{};
    _depth.setMinificationFilter(GL::SamplerFilter::Nearest)
        .setMagnificationFilter(GL::SamplerFilter::Nearest)
        .setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setStorage(1, GL::TextureFormat::Depth24Stencil8, e.framebufferSize());
    _depthFramebuffer.attachTexture(GL::Framebuffer::BufferAttachment::Depth, _depth, 0);

    _reinterpretDepth = GL::Renderbuffer{};
    _reinterpretDepth.setStorage(GL::RenderbufferFormat::RGBA8, e.framebufferSize());
    _reinterpretFramebuffer.attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, _reinterpretDepth);

    _reinterpretFramebuffer.setViewport({{}, e.framebufferSize()});
#endif
}

void MapViewer::keyPressEvent(KeyEvent& event) {
    if(_imgui.handleKeyPressEvent(event)) return;
        /* Reset the transformation to the original view */
    if(event.key() == KeyEvent::Key::NumZero) {
        resetCamera();
        redraw();
        return;
    }
}

void MapViewer::keyReleaseEvent(KeyEvent& event) {
    if(_imgui.handleKeyReleaseEvent(event)) return;
}

void MapViewer::mousePressEvent(MouseEvent& e) {
    if(_imgui.handleMousePressEvent(e)) return;
    const Float currentDepth = depthAt(e.position());
    const Float depth = currentDepth == 1.0f ? _lastDepth : currentDepth;
    _translationPoint = unproject(e.position(), depth);
    /* Update the rotation point only if we're not zooming against infinite
       depth or if the original rotation point is not yet initialized */
    if(currentDepth != 1.0f || _rotationPoint.isZero()) {
        _rotationPoint = _translationPoint;
        _lastDepth = depth;
    }
}

void MapViewer::mouseReleaseEvent(MouseEvent& e) {
    if(_imgui.handleMouseReleaseEvent(e)) return;
}

void MapViewer::mouseScrollEvent(MouseScrollEvent& e) {
    if(_imgui.handleMouseScrollEvent(e)) {
        /* Prevent scrolling the page */
        e.setAccepted();
        return;
    }
    e.setAccepted();
    const Float currentDepth = depthAt(e.position());
    const Float depth = currentDepth == 1.0f ? _lastDepth : currentDepth;
    const Vector3 p = unproject(e.position(), depth);
    /* Update the rotation point only if we're not zooming against infinite
       depth or if the original rotation point is not yet initialized */
    if(currentDepth != 1.0f || _rotationPoint.isZero()) {
        _rotationPoint = p;
        _lastDepth = depth;
    }

    const Float direction = e.offset().y();
    if(!direction) return;

    /* Move towards/backwards the rotation point in cam coords */
    _cameraObject->translateLocal(_rotationPoint*direction*0.1f);
    redraw();
}

void MapViewer::mouseMoveEvent(MouseMoveEvent &e) {
    if(_imgui.handleMouseMoveEvent(e)) return;

    if(_lastPosition == Vector2i{-1}) _lastPosition = e.position();
    const Vector2i delta = e.position() - _lastPosition;
    _lastPosition = e.position();

    if (!(e.buttons() & MouseMoveEvent::Button::Left)) return;

    /* Translate */
    if(e.modifiers() & MouseMoveEvent::Modifier::Shift) {
        const Vector3 p = unproject(e.position(), _lastDepth);
        _cameraObject->translateLocal(_translationPoint - p); /* is Z always 0? */
        _translationPoint = p;

    /* Rotate around rotation point */
    } else _cameraObject->transformLocal(
        Matrix4::translation(_rotationPoint)*
        Matrix4::rotationX(-0.005_radf*delta.y())*
        Matrix4::rotationY(-0.005_radf*delta.x())*
        Matrix4::translation(-_rotationPoint));

    redraw();
}

void MapViewer::textInputEvent(TextInputEvent& e) {
    if(_imgui.handleTextInputEvent(e)) return;
}

void MapViewer::resetBZDB() {
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
}

void MapViewer::init() {
    bzfsrand((unsigned int)time(0));
    
    resetBZDB();

    setErrorCallback(startupErrorCallback);

    CACHEMGR.loadIndex();
    CACHEMGR.limitCacheSize();
}

void MapViewer::loadMap(std::string path, const std::string& data, bool reloadEditor)
{
    worldSceneObjGen.destroyWorldObject();
    worldSceneBuilder.reset();

    DYNCOLORMGR.clear();
    TEXMATRIXMGR.clear();
    MAGNUMMATERIALMGR.clear(false);
    PHYDRVMGR.clear();
    TRANSFORMMGR.clear();
    OBSTACLEMGR.clear();
    MagnumTextureManager::instance().clear();

    resetBZDB();

    if (data == "") {
        BZWReader* reader = new BZWReader(path);
        auto worldInfo = reader->defineWorldFromFile();
        delete reader;
    } else {
        BZWReader* reader = new BZWReader(path, data);
        auto worldInfo = reader->defineWorldFromFile();
        delete reader;
    }

    if (reloadEditor) {
        editor.loadFile(path, data, data == "");
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

    TimeKeeper::setTick();

    // update the dynamic colors
    DYNCOLORMGR.update();

    // update the texture matrices
    TEXMATRIXMGR.update();

    // Needed for desktop. On emscripten this is a no-op
    cURLManager::perform();

    if (Downloads::requestFinalized()) {
        Downloads::finalizeDownloads();

        worldSceneObjGen.destroyWorldObject();
        worldSceneBuilder.reset();

        MAGNUMMATERIALMGR.forceLoadTextures();
        MAGNUMMATERIALMGR.rescanTextures();
        MagnumTextureManager::instance().disableAutomaticLoading();

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
    }
}

MAGNUM_APPLICATION_MAIN(MapViewer)