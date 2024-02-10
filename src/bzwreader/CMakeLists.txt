set(BZWREADER_SOURCES
    BZWError.cxx
    BZWReader.cxx
    CustomArc.cxx
    CustomBase.cxx
    CustomBox.cxx
    CustomCone.cxx
    CustomDynamicColor.cxx
    CustomGate.cxx
    CustomGroup.cxx
    CustomLink.cxx
    CustomMaterial.cxx
    CustomMesh.cxx
    CustomMeshFace.cxx
    CustomMeshTransform.cxx
    CustomPhysicsDriver.cxx
    CustomPyramid.cxx
    CustomSphere.cxx
    CustomTetra.cxx
    CustomTextureMatrix.cxx
    CustomWaterLevel.cxx
    CustomWeapon.cxx
    CustomWorld.cxx
    CustomZone.cxx
    EntryZones.cxx
    FlagInfo.cxx
    ParseMaterial.cxx
    TeamBases.cxx
    WorldFileLocation.cxx
    WorldFileObject.cxx
    WorldFileObstacle.cxx
    WorldInfo.cxx
    WorldWeapons.cxx
)

add_library(bzwreader STATIC ${BZWREADER_SOURCES})
target_include_directories(bzwreader PUBLIC include/)

target_link_libraries(bzwreader
${CURL_LIBRARIES}
)
