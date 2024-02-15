#include "GLInfo.h"

#include <Corrade/Utility/Debug.h>
#include <Corrade/Cpu.h>
#include <Corrade/Containers/StringIterable.h>
#include <Corrade/Utility/Debug.h>
#include <Magnum/Magnum.h>
#include <sstream>

#include "Magnum/GL/AbstractShaderProgram.h"
#include "Magnum/GL/Buffer.h"
#ifdef CORRADE_TARGET_EMSCRIPTEN
#include "Magnum/Platform/EmscriptenApplication.h"
#else
#include "Magnum/Platform/Sdl2Application.h"
#endif
#if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
#include "Magnum/GL/BufferTexture.h"
#endif
#include "Magnum/GL/Context.h"
#include "Magnum/GL/CubeMapTexture.h"
#if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
#include "Magnum/GL/CubeMapTextureArray.h"
#endif
#ifndef MAGNUM_TARGET_WEBGL
#include "Magnum/GL/DebugOutput.h"
#endif
#include "Magnum/GL/Extensions.h"
#include "Magnum/GL/Framebuffer.h"
#include "Magnum/GL/Mesh.h"
#if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
#include "Magnum/GL/MultisampleTexture.h"
#endif
#ifndef MAGNUM_TARGET_GLES
#include "Magnum/GL/RectangleTexture.h"
#endif
#include "Magnum/GL/Renderer.h"
#include "Magnum/GL/Renderbuffer.h"
#include "Magnum/GL/Shader.h"
#include "Magnum/GL/Texture.h"
#ifndef MAGNUM_TARGET_GLES2
#include "Magnum/GL/TextureArray.h"
#include "Magnum/GL/TransformFeedback.h"
#endif

#ifdef MAGNUM_TARGET_EGL
#include "Magnum/Platform/WindowlessEglApplication.h"
#elif defined(CORRADE_TARGET_IOS)
#include "Magnum/Platform/WindowlessIosApplication.h"
#elif defined(CORRADE_TARGET_APPLE)
#include "Magnum/Platform/WindowlessCglApplication.h"
#elif defined(CORRADE_TARGET_UNIX)
#include "Magnum/Platform/WindowlessGlxApplication.h"
#elif defined(CORRADE_TARGET_WINDOWS)
#include "Magnum/Platform/WindowlessWglApplication.h"
#else
#error no windowless application available on this platform
#endif

/* The __EMSCRIPTEN_major__ etc macros used to be passed implicitly, version
   3.1.4 moved them to a version header and version 3.1.23 dropped the
   backwards compatibility. To work consistently on all versions, including the
   header only if the version macros aren't present.
   https://github.com/emscripten-core/emscripten/commit/f99af02045357d3d8b12e63793cef36dfde4530a
   https://github.com/emscripten-core/emscripten/commit/f76ddc702e4956aeedb658c49790cc352f892e4c */
#if defined(CORRADE_TARGET_EMSCRIPTEN) && !defined(__EMSCRIPTEN_major__)
#include <emscripten/version.h>
#endif

using namespace Magnum;
using namespace Corrade;
using namespace Containers::Literals;

std::string getGLInfo() {

    std::ostringstream debugOut;
    {
    Utility::Debug redirectDebug{&debugOut};
    Debug{} << "";
    Debug{} << "  +---------------------------------------------------------+";
    Debug{} << "  |   Information about Magnum engine OpenGL capabilities   |";
    Debug{} << "  +---------------------------------------------------------+";
    Debug{} << "";

    #ifdef MAGNUM_WINDOWLESSEGLAPPLICATION_MAIN
    Debug{} << "Used application: Platform::WindowlessEglApplication";
    #elif defined(MAGNUM_WINDOWLESSIOSAPPLICATION_MAIN)
    Debug{} << "Used application: Platform::WindowlessIosApplication";
    #elif defined(MAGNUM_WINDOWLESSCGLAPPLICATION_MAIN)
    Debug{} << "Used application: Platform::WindowlessCglApplication";
    #elif defined(MAGNUM_WINDOWLESSGLXAPPLICATION_MAIN)
    Debug{} << "Used application: Platform::WindowlessGlxApplication";
    #elif defined(MAGNUM_WINDOWLESSWGLAPPLICATION_MAIN)
    Debug{} << "Used application: Platform::WindowlessWglApplication";
    #else
    #error no windowless application available on this platform
    #endif
    Debug{} << "Compilation flags:";
    #ifdef CORRADE_BUILD_DEPRECATED
    Debug{} << "    CORRADE_BUILD_DEPRECATED";
    #endif
    #ifdef CORRADE_BUILD_STATIC
    Debug{} << "    CORRADE_BUILD_STATIC";
    #endif
    #ifdef CORRADE_BUILD_MULTITHREADED
    Debug{} << "    CORRADE_BUILD_MULTITHREADED";
    #endif
    #ifdef CORRADE_TARGET_UNIX
    Debug{} << "    CORRADE_TARGET_UNIX";
    #endif
    #ifdef CORRADE_TARGET_APPLE
    Debug{} << "    CORRADE_TARGET_APPLE";
    #endif
    #ifdef CORRADE_TARGET_IOS
    Debug{} << "    CORRADE_TARGET_IOS";
    #endif
    #ifdef CORRADE_TARGET_IOS_SIMULATOR
    Debug{} << "    CORRADE_TARGET_IOS_SIMULATOR";
    #endif
    #ifdef CORRADE_TARGET_WINDOWS
    Debug{} << "    CORRADE_TARGET_WINDOWS";
    #endif
    #ifdef CORRADE_TARGET_WINDOWS_RT
    Debug{} << "    CORRADE_TARGET_WINDOWS_RT";
    #endif
    #ifdef CORRADE_TARGET_EMSCRIPTEN
    Debug{} << "    CORRADE_TARGET_EMSCRIPTEN (" << Debug::nospace
        << __EMSCRIPTEN_major__ << Debug::nospace << "." << Debug::nospace
        << __EMSCRIPTEN_minor__ << Debug::nospace << "." << Debug::nospace
        << __EMSCRIPTEN_tiny__ << Debug::nospace << ")";
    #endif
    #ifdef CORRADE_TARGET_ANDROID
    Debug{} << "    CORRADE_TARGET_ANDROID";
    #endif
    #ifdef CORRADE_TARGET_X86
    Debug{} << "    CORRADE_TARGET_X86";
    #endif
    #ifdef CORRADE_TARGET_ARM
    Debug{} << "    CORRADE_TARGET_ARM";
    #endif
    #ifdef CORRADE_TARGET_POWERPC
    Debug{} << "    CORRADE_TARGET_POWERPC";
    #endif
    #ifdef CORRADE_TARGET_WASM
    Debug{} << "    CORRADE_TARGET_WASM";
    #endif
    #ifdef CORRADE_TARGET_32BIT
    Debug{} << "    CORRADE_TARGET_32BIT";
    #endif
    #ifdef CORRADE_TARGET_BIG_ENDIAN
    Debug{} << "    CORRADE_TARGET_BIG_ENDIAN";
    #endif
    #ifdef CORRADE_TARGET_GCC
    Debug{} << "    CORRADE_TARGET_GCC";
    #endif
    #ifdef CORRADE_TARGET_CLANG
    Debug{} << "    CORRADE_TARGET_CLANG";
    #endif
    #ifdef CORRADE_TARGET_APPLE_CLANG
    Debug{} << "    CORRADE_TARGET_APPLE_CLANG";
    #endif
    #ifdef CORRADE_TARGET_CLANG_CL
    Debug{} << "    CORRADE_TARGET_CLANG_CL";
    #endif
    #ifdef CORRADE_TARGET_MSVC
    Debug{} << "    CORRADE_TARGET_MSVC";
    #endif
    #ifdef CORRADE_TARGET_MINGW
    Debug{} << "    CORRADE_TARGET_MINGW";
    #endif
    #ifdef CORRADE_TARGET_LIBCXX
    Debug{} << "    CORRADE_TARGET_LIBCXX";
    #endif
    #ifdef CORRADE_TARGET_LIBSTDCXX
    Debug{} << "    CORRADE_TARGET_LIBSTDCXX";
    #endif
    #ifdef CORRADE_TARGET_DINKUMWARE
    Debug{} << "    CORRADE_TARGET_DINKUMWARE";
    #endif
    #ifdef CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT
    Debug{} << "    CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT";
    #endif
    #ifdef CORRADE_TESTSUITE_TARGET_XCTEST
    Debug{} << "    CORRADE_TESTSUITE_TARGET_XCTEST";
    #endif
    #ifdef CORRADE_UTILITY_USE_ANSI_COLORS
    Debug{} << "    CORRADE_UTILITY_USE_ANSI_COLORS";
    #endif
    #ifdef MAGNUM_BUILD_DEPRECATED
    Debug{} << "    MAGNUM_BUILD_DEPRECATED";
    #endif
    #ifdef MAGNUM_BUILD_STATIC
    Debug{} << "    MAGNUM_BUILD_STATIC";
    #endif
    #ifdef MAGNUM_TARGET_GLES
    Debug{} << "    MAGNUM_TARGET_GLES";
    #endif
    #ifdef MAGNUM_TARGET_GLES2
    Debug{} << "    MAGNUM_TARGET_GLES2";
    #endif
    #ifdef MAGNUM_TARGET_WEBGL
    Debug{} << "    MAGNUM_TARGET_WEBGL";
    #endif
    #ifdef MAGNUM_TARGET_EGL
    Debug{} << "    MAGNUM_TARGET_EGL";
    #endif
    Debug{} << "Compiled CPU features:";
    Debug{} << "   " << Debug::packed << Cpu::compiledFeatures();
    #ifdef CORRADE_BUILD_CPU_RUNTIME_DISPATCH
    Debug{} << "Detected CPU features:";
    Debug{} << "   " << Debug::packed << Cpu::runtimeFeatures();
    #endif
    Debug{} << "";

    GL::Context& c = GL::Context::current();

    Debug{} << "";

    #ifndef MAGNUM_TARGET_GLES
    Debug{} << "Core profile:" << (c.isCoreProfile() ? "yes" : "no");
    #endif
    #ifndef MAGNUM_TARGET_WEBGL
    Debug{} << "Context flags:" << Debug::packed << c.flags();
    #endif
    Debug{} << "Detected driver:" << Debug::packed << c.detectedDriver();

    Debug{} << "Supported GLSL versions:";
    Debug{} << "   " << ", "_s.joinWithoutEmptyParts(c.shadingLanguageVersionStrings());

    /*if(true) {
        Debug{} << "Extension strings:";
        for(Containers::StringView e: c.extensionStrings())
            Debug{} << "   " << e;
        return;
    }*/

    Debug{} << "";

    /* Get first future (not supported) version */
    GL::Version versions[]{
        #ifndef MAGNUM_TARGET_GLES
        GL::Version::GL300,
        GL::Version::GL310,
        GL::Version::GL320,
        GL::Version::GL330,
        GL::Version::GL400,
        GL::Version::GL410,
        GL::Version::GL420,
        GL::Version::GL430,
        GL::Version::GL440,
        GL::Version::GL450,
        GL::Version::GL460,
        #else
        GL::Version::GLES300,
        #ifndef MAGNUM_TARGET_WEBGL
        GL::Version::GLES310,
        GL::Version::GLES320,
        #endif
        #endif
        GL::Version::None
    };
    std::size_t future = 0;

    while(versions[future] != GL::Version::None && c.isVersionSupported(versions[future]))
        ++future;

    constexpr Containers::StringView NewlineAndManySpaces =
        /* 123456789_123456789_123456789_123456789_123456789_123456789_ */
        "\n                                                            "
          "        "_s;

    /* Display supported OpenGL extensions from unsupported versions */
    for(std::size_t i = future; i != Containers::arraySize(versions); ++i) {
        if(versions[i] != GL::Version::None)
            Debug{} << versions[i] << "extension support:";
        else Debug{} << "Vendor extension support:";

        for(const auto& extension: GL::Extension::extensions(versions[i])) {
            Containers::StringView extensionName = extension.string();
            Debug d;
            d << "   " << extensionName << NewlineAndManySpaces.slice(1, 61 - extensionName.size());
            if(c.isExtensionSupported(extension))
                d << "SUPPORTED";
            else if(c.isExtensionDisabled(extension))
                d << " removed";
            else if(c.isVersionSupported(extension.requiredVersion()))
                d << "    -";
            else
                d << "   n/a";
        }

        Debug{} << "";
    }

    /* Limits and implementation-defined values */
    #define _h(val) Debug{} << "\n " << GL::Extensions::val::string() << Debug::nospace << ":";
    #define _l(val) Debug{} << "   " << #val << Debug::nospace << (sizeof(#val) > 64 ? NewlineAndManySpaces.slice(0, 69) : NewlineAndManySpaces.slice(1, 66 - sizeof(#val))) << val;
    #define _lvec(val) Debug{} << "   " << #val << Debug::nospace << (sizeof(#val) > 48 ? NewlineAndManySpaces.slice(0, 53) : NewlineAndManySpaces.slice(1, 50 - sizeof(#val))) << Debug::packed << val;

    Debug{} << "Limits and implementation-defined values:";
    _lvec(GL::AbstractFramebuffer::maxViewportSize())
    _l(GL::AbstractFramebuffer::maxDrawBuffers())
    _l(GL::Framebuffer::maxColorAttachments())
    _l(GL::Mesh::maxVertexAttributeStride())
    #ifndef MAGNUM_TARGET_GLES2
    _l(GL::Mesh::maxElementIndex())
    _l(GL::Mesh::maxElementsIndices())
    _l(GL::Mesh::maxElementsVertices())
    #endif
    _lvec(GL::Renderer::lineWidthRange())
    #if !(defined(MAGNUM_TARGET_GLES2) && defined(MAGNUM_TARGET_WEBGL))
    _l(GL::Renderer::maxClipDistances())
    #endif
    _l(GL::Renderbuffer::maxSize())
    #if !(defined(MAGNUM_TARGET_WEBGL) && defined(MAGNUM_TARGET_GLES2))
    _l(GL::Renderbuffer::maxSamples())
    #endif
    _l(GL::Shader::maxVertexOutputComponents())
    _l(GL::Shader::maxFragmentInputComponents())
    _l(GL::Shader::maxTextureImageUnits(GL::Shader::Type::Vertex))
    #if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    _l(GL::Shader::maxTextureImageUnits(GL::Shader::Type::TessellationControl))
    _l(GL::Shader::maxTextureImageUnits(GL::Shader::Type::TessellationEvaluation))
    _l(GL::Shader::maxTextureImageUnits(GL::Shader::Type::Geometry))
    _l(GL::Shader::maxTextureImageUnits(GL::Shader::Type::Compute))
    #endif
    _l(GL::Shader::maxTextureImageUnits(GL::Shader::Type::Fragment))
    _l(GL::Shader::maxCombinedTextureImageUnits())
    _l(GL::Shader::maxUniformComponents(GL::Shader::Type::Vertex))
    #if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    _l(GL::Shader::maxUniformComponents(GL::Shader::Type::TessellationControl))
    _l(GL::Shader::maxUniformComponents(GL::Shader::Type::TessellationEvaluation))
    _l(GL::Shader::maxUniformComponents(GL::Shader::Type::Geometry))
    _l(GL::Shader::maxUniformComponents(GL::Shader::Type::Compute))
    #endif
    _l(GL::Shader::maxUniformComponents(GL::Shader::Type::Fragment))
    _l(GL::AbstractShaderProgram::maxVertexAttributes())
    #ifndef MAGNUM_TARGET_GLES2
    _l(GL::AbstractTexture::maxLodBias())
    #endif
    #ifndef MAGNUM_TARGET_GLES
    _lvec(GL::Texture1D::maxSize())
    #endif
    _lvec(GL::Texture2D::maxSize())
    #ifndef MAGNUM_TARGET_GLES2
    _lvec(GL::Texture3D::maxSize()) /* Checked ES2 version below */
    #endif
    _lvec(GL::CubeMapTexture::maxSize())

    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::blend_func_extended>()) {
        _h(ARB::blend_func_extended)

        _l(GL::AbstractFramebuffer::maxDualSourceDrawBuffers())
    }
    #endif

    #ifndef MAGNUM_TARGET_GLES2
    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::cull_distance>())
    #else
    if(c.isExtensionSupported<GL::Extensions::EXT::clip_cull_distance>())
    #endif
    {
        #ifndef MAGNUM_TARGET_GLES
        _h(ARB::cull_distance)
        #else
        _h(EXT::clip_cull_distance)
        #endif

        _l(GL::Renderer::maxCullDistances())
        _l(GL::Renderer::maxCombinedClipAndCullDistances())
    }
    #endif

    #if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::compute_shader>())
    #endif
    {
        #ifndef MAGNUM_TARGET_GLES
        _h(ARB::compute_shader)
        #endif

        _l(GL::AbstractShaderProgram::maxComputeSharedMemorySize())
        _l(GL::AbstractShaderProgram::maxComputeWorkGroupInvocations())
        _lvec(GL::AbstractShaderProgram::maxComputeWorkGroupCount())
        _lvec(GL::AbstractShaderProgram::maxComputeWorkGroupSize())
    }

    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::explicit_uniform_location>())
    #endif
    {
        #ifndef MAGNUM_TARGET_GLES
        _h(ARB::explicit_uniform_location)
        #endif

        _l(GL::AbstractShaderProgram::maxUniformLocations())
    }
    #endif

    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::map_buffer_alignment>()) {
        _h(ARB::map_buffer_alignment)

        _l(GL::Buffer::minMapAlignment())
    }
    #endif

    #if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::shader_atomic_counters>())
    #endif
    {
        #ifndef MAGNUM_TARGET_GLES
        _h(ARB::shader_atomic_counters)
        #endif

        _l(GL::Buffer::maxAtomicCounterBindings())
        _l(GL::Shader::maxAtomicCounterBuffers(GL::Shader::Type::Vertex))
        _l(GL::Shader::maxAtomicCounterBuffers(GL::Shader::Type::TessellationControl))
        _l(GL::Shader::maxAtomicCounterBuffers(GL::Shader::Type::TessellationEvaluation))
        _l(GL::Shader::maxAtomicCounterBuffers(GL::Shader::Type::Geometry))
        _l(GL::Shader::maxAtomicCounterBuffers(GL::Shader::Type::Compute))
        _l(GL::Shader::maxAtomicCounterBuffers(GL::Shader::Type::Fragment))
        _l(GL::Shader::maxCombinedAtomicCounterBuffers())
        _l(GL::Shader::maxAtomicCounters(GL::Shader::Type::Vertex))
        _l(GL::Shader::maxAtomicCounters(GL::Shader::Type::TessellationControl))
        _l(GL::Shader::maxAtomicCounters(GL::Shader::Type::TessellationEvaluation))
        _l(GL::Shader::maxAtomicCounters(GL::Shader::Type::Geometry))
        _l(GL::Shader::maxAtomicCounters(GL::Shader::Type::Compute))
        _l(GL::Shader::maxAtomicCounters(GL::Shader::Type::Fragment))
        _l(GL::Shader::maxCombinedAtomicCounters())
        _l(GL::AbstractShaderProgram::maxAtomicCounterBufferSize())
    }

    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::shader_image_load_store>())
    #endif
    {
        #ifndef MAGNUM_TARGET_GLES
        _h(ARB::shader_image_load_store)
        #endif

        _l(GL::Shader::maxImageUniforms(GL::Shader::Type::Vertex))
        _l(GL::Shader::maxImageUniforms(GL::Shader::Type::TessellationControl))
        _l(GL::Shader::maxImageUniforms(GL::Shader::Type::TessellationEvaluation))
        _l(GL::Shader::maxImageUniforms(GL::Shader::Type::Geometry))
        _l(GL::Shader::maxImageUniforms(GL::Shader::Type::Compute))
        _l(GL::Shader::maxImageUniforms(GL::Shader::Type::Fragment))
        _l(GL::Shader::maxCombinedImageUniforms())
        _l(GL::AbstractShaderProgram::maxCombinedShaderOutputResources())
        _l(GL::AbstractShaderProgram::maxImageUnits())
        #ifndef MAGNUM_TARGET_GLES
        _l(GL::AbstractShaderProgram::maxImageSamples())
        #endif
    }

    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::shader_storage_buffer_object>())
    #endif
    {
        #ifndef MAGNUM_TARGET_GLES
        _h(ARB::shader_storage_buffer_object)
        #endif

        _l(GL::Buffer::shaderStorageOffsetAlignment())
        _l(GL::Buffer::maxShaderStorageBindings())
        _l(GL::Shader::maxShaderStorageBlocks(GL::Shader::Type::Vertex))
        _l(GL::Shader::maxShaderStorageBlocks(GL::Shader::Type::TessellationControl))
        _l(GL::Shader::maxShaderStorageBlocks(GL::Shader::Type::TessellationEvaluation))
        _l(GL::Shader::maxShaderStorageBlocks(GL::Shader::Type::Geometry))
        _l(GL::Shader::maxShaderStorageBlocks(GL::Shader::Type::Compute))
        _l(GL::Shader::maxShaderStorageBlocks(GL::Shader::Type::Fragment))
        _l(GL::Shader::maxCombinedShaderStorageBlocks())
        /* AbstractShaderProgram::maxCombinedShaderOutputResources() already in shader_image_load_store */
        _l(GL::AbstractShaderProgram::maxShaderStorageBlockSize())
    }
    #endif

    #if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::texture_multisample>())
    #endif
    {
        #ifndef MAGNUM_TARGET_GLES
        _h(ARB::texture_multisample)
        #endif

        _l(GL::AbstractTexture::maxColorSamples())
        _l(GL::AbstractTexture::maxDepthSamples())
        _l(GL::AbstractTexture::maxIntegerSamples())
        _lvec(GL::MultisampleTexture2D::maxSize())
        _lvec(GL::MultisampleTexture2DArray::maxSize())
    }
    #endif

    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::texture_rectangle>()) {
        _h(ARB::texture_rectangle)

        _lvec(GL::RectangleTexture::maxSize())
    }
    #endif

    #ifndef MAGNUM_TARGET_GLES2
    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::uniform_buffer_object>())
    #endif
    {
        #ifndef MAGNUM_TARGET_GLES
        _h(ARB::uniform_buffer_object)
        #endif

        _l(GL::Buffer::uniformOffsetAlignment())
        _l(GL::Buffer::maxUniformBindings())
        _l(GL::Shader::maxUniformBlocks(GL::Shader::Type::Vertex))
        #ifndef MAGNUM_TARGET_WEBGL
        _l(GL::Shader::maxUniformBlocks(GL::Shader::Type::TessellationControl))
        _l(GL::Shader::maxUniformBlocks(GL::Shader::Type::TessellationEvaluation))
        _l(GL::Shader::maxUniformBlocks(GL::Shader::Type::Geometry))
        _l(GL::Shader::maxUniformBlocks(GL::Shader::Type::Compute))
        #endif
        _l(GL::Shader::maxUniformBlocks(GL::Shader::Type::Fragment))
        _l(GL::Shader::maxCombinedUniformBlocks())
        _l(GL::Shader::maxCombinedUniformComponents(GL::Shader::Type::Vertex))
        #ifndef MAGNUM_TARGET_WEBGL
        _l(GL::Shader::maxCombinedUniformComponents(GL::Shader::Type::TessellationControl))
        _l(GL::Shader::maxCombinedUniformComponents(GL::Shader::Type::TessellationEvaluation))
        _l(GL::Shader::maxCombinedUniformComponents(GL::Shader::Type::Geometry))
        _l(GL::Shader::maxCombinedUniformComponents(GL::Shader::Type::Compute))
        #endif
        _l(GL::Shader::maxCombinedUniformComponents(GL::Shader::Type::Fragment))
        _l(GL::AbstractShaderProgram::maxUniformBlockSize())
    }

    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::EXT::gpu_shader4>())
    #endif
    {
        #ifndef MAGNUM_TARGET_GLES
        _h(EXT::gpu_shader4)
        #endif

        _l(GL::AbstractShaderProgram::minTexelOffset())
        _l(GL::AbstractShaderProgram::maxTexelOffset())
    }

    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::EXT::texture_array>())
    #endif
    {
        #ifndef MAGNUM_TARGET_GLES
        _h(EXT::texture_array)
        #endif

        #ifndef MAGNUM_TARGET_GLES
        _lvec(GL::Texture1DArray::maxSize())
        #endif
        _lvec(GL::Texture2DArray::maxSize())
    }
    #endif

    #ifndef MAGNUM_TARGET_GLES2
    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::EXT::transform_feedback>())
    #endif
    {
        #ifndef MAGNUM_TARGET_GLES
        _h(EXT::transform_feedback)
        #endif

        _l(GL::TransformFeedback::maxInterleavedComponents())
        _l(GL::TransformFeedback::maxSeparateAttributes())
        _l(GL::TransformFeedback::maxSeparateComponents())
    }
    #endif

    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::transform_feedback3>()) {
        _h(ARB::transform_feedback3)

        _l(GL::TransformFeedback::maxBuffers())
        _l(GL::TransformFeedback::maxVertexStreams())
    }
    #endif

    #if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::geometry_shader4>())
    #else
    if(c.isExtensionSupported<GL::Extensions::EXT::geometry_shader>())
    #endif
    {
        #ifndef MAGNUM_TARGET_GLES
        _h(ARB::geometry_shader4)
        #else
        _h(EXT::geometry_shader)
        #endif

        _l(GL::AbstractShaderProgram::maxGeometryOutputVertices())
        _l(GL::Shader::maxGeometryInputComponents())
        _l(GL::Shader::maxGeometryOutputComponents())
        _l(GL::Shader::maxGeometryTotalOutputComponents())
    }
    #endif

    #if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::tessellation_shader>())
    #else
    if(c.isExtensionSupported<GL::Extensions::EXT::tessellation_shader>())
    #endif
    {
        #ifndef MAGNUM_TARGET_GLES
        _h(ARB::tessellation_shader)
        #else
        _h(EXT::tessellation_shader)
        #endif

        _l(GL::Shader::maxTessellationControlInputComponents())
        _l(GL::Shader::maxTessellationControlOutputComponents())
        _l(GL::Shader::maxTessellationControlTotalOutputComponents())
        _l(GL::Shader::maxTessellationEvaluationInputComponents())
        _l(GL::Shader::maxTessellationEvaluationOutputComponents())
        _l(GL::Renderer::maxPatchVertexCount())
    }
    #endif

    #if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::texture_buffer_object>())
    #else
    if(c.isExtensionSupported<GL::Extensions::EXT::texture_buffer>())
    #endif
    {
        #ifndef MAGNUM_TARGET_GLES
        _h(ARB::texture_buffer_object)
        #else
        _h(EXT::texture_buffer)
        #endif

        _l(GL::BufferTexture::maxSize())
    }

    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::texture_buffer_range>())
    #else
    if(c.isExtensionSupported<GL::Extensions::EXT::texture_buffer>())
    #endif
    {
        #ifndef MAGNUM_TARGET_GLES
        _h(ARB::texture_buffer_range)
        #else
        /* Header added above */
        #endif

        _l(GL::BufferTexture::offsetAlignment())
    }

    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::texture_cube_map_array>())
    #else
    if(c.isExtensionSupported<GL::Extensions::EXT::texture_cube_map_array>())
    #endif
    {
        #ifndef MAGNUM_TARGET_GLES
        _h(ARB::texture_cube_map_array)
        #else
        _h(EXT::texture_cube_map_array)
        #endif

        _lvec(GL::CubeMapTextureArray::maxSize())
    }
    #endif

    #ifndef MAGNUM_TARGET_GLES
    if(c.isExtensionSupported<GL::Extensions::ARB::texture_filter_anisotropic>()) {
        _h(ARB::texture_filter_anisotropic)

        _l(GL::Sampler::maxMaxAnisotropy())
    } else
    #endif
    if(c.isExtensionSupported<GL::Extensions::EXT::texture_filter_anisotropic>()) {
        _h(EXT::texture_filter_anisotropic)

        _l(GL::Sampler::maxMaxAnisotropy())
    }

    #ifndef MAGNUM_TARGET_WEBGL
    if(c.isExtensionSupported<GL::Extensions::KHR::debug>()) {
        _h(KHR::debug)

        _l(GL::AbstractObject::maxLabelLength())
        _l(GL::DebugOutput::maxLoggedMessages())
        _l(GL::DebugOutput::maxMessageLength())
        _l(GL::DebugGroup::maxStackDepth())
    }
    #endif

    #if defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    if(c.isExtensionSupported<GL::Extensions::OES::texture_3D>()) {
        _h(OES::texture_3D)

        _lvec(GL::Texture3D::maxSize())
    }
    #endif

    #undef _l
    #undef _h
    }
    return debugOut.str();
}