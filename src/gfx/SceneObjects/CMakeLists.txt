set(SCENEOBJ_SOURCES
    TankObjectBuilder.cpp
)

target_sources(bzgfx PRIVATE ${SCENEOBJ_SOURCES})
target_include_directories(bzgfx PUBLIC include/)
