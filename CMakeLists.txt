cmake_minimum_required(VERSION 3.18.4 FATAL_ERROR)

include(CheckSymbolExists)
include(CheckCXXSymbolExists)
include(CheckFunctionExists)
include(CheckTypeSize)

# Add our extra CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

include(BZUtils)

# Define the project information
cmake_policy(SET CMP0048 NEW) # This enables new behavior for VERSION below
project(BZ-Next
        VERSION 0.1.00
        DESCRIPTION "Free multi-player 3D tank battle game for Windows, Linux, and macOS"
	HOMEPAGE_URL "https://bz-next.github.io/"
)

add_subdirectory(corrade EXCLUDE_FROM_ALL)

set(MAGNUM_WITH_ANYIMAGEIMPORTER ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_ANYSCENEIMPORTER ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_OBJIMPORTER ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_PNGIMPORTER ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_SDL2APPLICATION ON CACHE BOOL "" FORCE)
set(MAGNUM_WITH_IMGUI ON CACHE BOOL "" FORCE)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

add_subdirectory(magnum EXCLUDE_FROM_ALL)
add_subdirectory(magnum-integration EXCLUDE_FROM_ALL)
add_subdirectory(magnum-plugins EXCLUDE_FROM_ALL)

find_package(Corrade REQUIRED Main)
find_package(Magnum REQUIRED
    GL
    Shaders
    Primitives
    MeshTools
    Trade
    Sdl2Application
    DebugTools)

find_package(MagnumIntegration REQUIRED ImGui)
find_package(MagnumPlugins REQUIRED PngImporter)


# Compiler tests directory
set(COMPILER_TESTS_DIR "${PROJECT_SOURCE_DIR}/cmake/tests")

string(TOLOWER ${CMAKE_PROJECT_NAME} PROJECT_NAME_LOWER)

# Require C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)


set(ENABLE_CLIENT TRUE CACHE BOOL "Enable the graphical client")
set(ENABLE_ROBOTS TRUE CACHE BOOL "Enable robot players") # Is ths only in the client?
set(ENABLE_SERVER TRUE CACHE BOOL "Enable the server")
set(ENABLE_PLUGINS TRUE CACHE BOOL "Enable plugins in the server")
set(ENABLE_BZADMIN TRUE CACHE BOOL "Enable the text client")

# TODO: Just use ENABLE_PLUGINS in the code? Or just always enable them?
if (ENABLE_PLUGINS)
    set(BZ_PLUGINS TRUE)
else()
    set(BZ_PLUGINS FALSE)
endif()


math(EXPR VERSION_MINOR_IS_ODD "${CMAKE_PROJECT_VERSION_MINOR} % 2")
math(EXPR VERSION_PATCH_IS_ODD "${CMAKE_PROJECT_VERSION_PATCH} % 2")

set(BUILD_RC_NUMBER "" CACHE STRING "Define the release candidate number")

if(VERSION_MINOR_IS_ODD OR VERSION_PATCH_IS_ODD)
    set(BUILD_TYPE "DEVEL" CACHE STRING "Define the build type (DEVEL, RCn, STABLE, or MAINT)" FORCE)
elseif(NOT VERSION_MINOR_IS_ODD AND NOT VERSION_PATCH_IS_ODD AND BUILD_RC_NUMBER)
    set(BUILD_TYPE "RC" CACHE STRING "Define the build type (DEVEL, RC, STABLE, or MAINT)" FORCE)
elseif(CMAKE_PROJECT_VERSION_PATCH EQUAL 0)
    set(BUILD_TYPE "STABLE" CACHE STRING "Define the build type (DEVEL, RCn, STABLE, or MAINT)" FORCE)
else()
    set(BUILD_TYPE "MAINT" CACHE STRING "Define the build type (DEVEL, RCn, STABLE, or MAINT)" FORCE)
endif()

if(BUILD_TYPE STREQUAL "RC")
    set(BZ_BUILD_TYPE "RC${BUILD_RC_NUMBER}")
else()
    set(BZ_BUILD_TYPE "${BUILD_TYPE}")
endif()


# Needed for everything
find_package(CURL REQUIRED COMPONENTS HTTP HTTPS SSL IPv6 AsynchDNS libz)
find_package(c-ares REQUIRED)
find_package(ZLIB REQUIRED)

# Optional for BZAdmin
find_package(Curses)

# MinGW Specific
if (MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
endif()

if(ENABLE_CLIENT)
    # Needed for client
    find_package(SDL2)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads)
    # TODO: Check if this is an okay way to test for pthreads
    set(HAVE_PTHREADS ${CMAKE_USE_PTHREADS_INIT})
    find_package(OpenGL)
    find_package(GLEW)

    # Probably a better way to handle this, but it works for now...
    if (MINGW)
        link_libraries(${GLEW_LIBRARIES})
        link_libraries(png)
        link_libraries(systre)
        link_libraries(zlibstatic)
        link_libraries(winmm)
        link_libraries(regex)
        link_libraries(tre)
        link_libraries(winpthread)
        link_libraries(wsock32)
        link_libraries(ws2_32)
    endif()

    # Check if all our dependencies were found, and if not, disable the client
    if(NOT SDL2_FOUND OR NOT Threads_FOUND OR NOT OPENGL_FOUND OR NOT GLEW_FOUND)
        set(CLIENT_INCLUDED FALSE)
        if(NOT SDL2_FOUND)
            message(WARNING "Client was enabled, but will not be built because SDL2 is missing.")
        endif(NOT SDL2_FOUND)
        if(NOT Threads_FOUND)
            message(WARNING "Client was enabled, but will not be built because threads are missing.")
        endif(NOT Threads_FOUND)
        if(NOT OPENGL_FOUND)
            message(WARNING "Client was enabled, but will not be built because OpenGL is missing.")
        endif(NOT OPENGL_FOUND)
        if(NOT GLEW_FOUND)
            message(WARNING "Client was enabled, but will not be built because GLEW is missing.")
        endif(NOT GLEW_FOUND)
    else(NOT SDL2_FOUND OR NOT Threads_FOUND OR NOT OPENGL_FOUND OR NOT GLEW_FOUND)
        set(CLIENT_INCLUDED TRUE)
    endif(NOT SDL2_FOUND OR NOT Threads_FOUND OR NOT OPENGL_FOUND OR NOT GLEW_FOUND)

    add_compile_definitions(
            HAVE_SDL
            HAVE_SDL2
    )
else(CLIENT_ENABLED)
    set(CLIENT_INCLUDED FALSE)
endif(ENABLE_CLIENT)

string(TIMESTAMP NOW "%Y-%m-%d")
set(BUILD_DATE "${NOW}" CACHE STRING "Date of the build. Defaults to the date the build was configured. Useful for reproducable builds.")

# TODO: Set BZFLAG_DATA based on the OS? Need to check if this is used on all platforms
set(BZFLAG_DATA "/usr/local/share/${PROJECT_NAME_LOWER}" CACHE STRING "Location of installed game data assets.")

#if(LINUX)
#    set(BZFLAG_BUILD_OS "linux")
#elseif(WIN32)
#    if(MSVC)
#        set(BZFLAG_BUILD_OS "windows-msvc-${MSVC_VERSION}")
#    else()
#        set(BZFLAG_BUILD_OS "windows")
#    endif()
#elseif(BSD)
    # Added in CMake 2.25, this value can be one of the following: DragonFlyBSD, FreeBSD, OpenBSD, or NetBSD
#    set(BZFLAG_BUILD_OS "${BSD}")
#else()
#    set(BZFLAG_BUILD_OS "${CMAKE_SYSTEM_NAME}")
#endif()

# Not cached so it doesn't show up in the list of options
set(BZFLAG_BUILD_OS "${CMAKE_SYSTEM_NAME}")
if(CMAKE_CXX_COMPILER_ID)
    set(BZFLAG_BUILD_OS "${BZFLAG_BUILD_OS}-${CMAKE_CXX_COMPILER_ID}")
endif()
if(MSVC)
    set(BZFLAG_BUILD_OS "${BZFLAG_BUILD_OS}-${MSVC_VERSION}")
elseif(CMAKE_CXX_COMPILER_VERSION)
    set(BZFLAG_BUILD_OS "${BZFLAG_BUILD_OS}-${CMAKE_CXX_COMPILER_VERSION}")
endif()
if(XCODE)
    set(BZFLAG_BUILD_OS "${BZFLAG_BUILD_OS}-XCode-${XCODE_VERSION}")
endif()

message(${BZFLAG_BUILD_OS})

add_compile_definitions(
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Debug>:DEBUG_RENDERING>
    $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
)

# Check for some various math functions
if(UNIX)
    set(CMAKE_REQUIRED_LIBRARIES m)
endif()

# Define the install directory for libraries
# TODO: Check if this should be platform specific
add_compile_definitions(INSTALL_LIB_DIR="${CMAKE_INSTALL_PREFIX}/lib")

# Add both our normal include directory and the include directory with our generated headers
include_directories(BEFORE
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_BINARY_DIR}/include
)

set(INSTALL_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/bzflag")

add_compile_definitions(
    # TODO: Should this be platform specific?
    INSTALL_DATA_DIR="${INSTALL_DATA_DIR}"
)

add_subdirectory(src)
if(BZ_PLUGINS)
    add_subdirectory(plugins)
endif()

#install(TARGETS bzflag bzadmin bzfs)
add_subdirectory(data)

# Check for some C headers
list(APPEND C_HEADERS_TO_CHECK
    limits.h
    memory.h
# Disabling this for now...
#    miniupnpc/miniupnpc.h # TODO: Should this be part of a find_package?
    ncurses.h
    regex.h
    stdlib.h
    stdint.h
    stdlib.h
    strings.h
    string.h
    sys/param.h
    sys/socket.h
    sys/stat.h
    sys/types.h
    unistd.h
    values.h
    inttypes.h
)

foreach(c_header_to_check ${C_HEADERS_TO_CHECK})
    TO_C_DEFINE(def "${c_header_to_check}")
    check_include_file("${c_header_to_check}" "${def}")
endforeach()

# Check for some C++ headers
list(APPEND CXX_HEADERS_TO_CHECK
    cmath
    cstdio
    cstring
    cstdlib
)
foreach(cxx_header_to_check ${CXX_HEADERS_TO_CHECK})
    TO_C_DEFINE(def "${cxx_header_to_check}")
    check_include_file_cxx("${cxx_header_to_check}" ${def})
endforeach()

# We absolutely require regex.h -- though there's a #define in config.h, the
# build breaks without it!
if(NOT HAVE_REGEX_H)
    message("regex.h is required.")
    if (MINGW)
        message("MinGW users, it is suggested to install libtre and libsystre.")
        message("Instructions available at https://bz-next.github.io/building/")
        message(FATAL_ERROR "Build terminating...")
    endif()
endif()

list(APPEND LIBM_FUNCTIONS_TO_CHECK
    acosf
    asinf
    atan2f
    atanf
    cosf
    expf
    fabsf
    floorf
    fmodf
    hypotf
    log10f
    logf
    powf
    sinf
    sqrtf
    tanf
)

foreach(libm_function_to_check ${LIBM_FUNCTIONS_TO_CHECK})
    TO_C_DEFINE(def ${libm_function_to_check})
    check_symbol_exists("${libm_function_to_check}" "math.h" ${def})
endforeach()


COMPILE_CXX_TEST_PROG(std::isnan)
if(NOT HAVE_STD__ISNAN)
    COMPILE_CXX_TEST_PROG(isnan)
endif()

COMPILE_CXX_TEST_PROG(std::min)
COMPILE_CXX_TEST_PROG(std::max)
COMPILE_CXX_TEST_PROG(std::count)

check_symbol_exists("select" "sys/select.h" HAVE_SELECT)

# Check for some other functions
check_symbol_exists(Sleep "" HAVE_SLEEP)
check_symbol_exists(snooze "" HAVE_SNOOZE)
check_symbol_exists(usleep "unistd.h" HAVE_USLEEP)
check_cxx_symbol_exists(vsnprintf "cstdio" HAVE_VSNPRINTF)
check_symbol_exists(WaitForSingleObject "" HAVE_WAITFORSINGLEOBJECT)
check_symbol_exists(_stricmp "" HAVE__STRICMP)
check_symbol_exists(_strnicmp "" HAVE__STRNICMP)
check_symbol_exists(_vsnprintf "" HAVE__VSNPRINTF)


# Generate some files
configure_file(include/config.h.in include/config.h)