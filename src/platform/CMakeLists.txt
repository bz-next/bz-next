add_library(bzplatform
    BzfDisplay.cxx
    BzfJoystick.cxx
    BzfMedia.cxx
    BzfVisual.cxx
    BzfWindow.cxx
    PlatformFactory.cxx
    SDL2Display.cxx
    SDL2Display.h
    SDL2Visual.cxx
    SDL2Visual.h
    SDL2Window.cxx
    SDL2Window.h
    SDLJoystick.cxx
    SDLJoystick.h
    SDLMedia.cxx
    SDLMedia.h
    SDLPlatformFactory.cxx
    SDLPlatformFactory.h
)

if(APPLE)
    target_sources(bzplatform PRIVATE
        MacDataPath.cxx
        SDLMain.m
        SDLMain.h
    )
endif(APPLE)

include_directories(${SDL2_INCLUDE_DIR})

target_link_libraries(bzplatform
    ${SDL2_LIBRARY}
)
