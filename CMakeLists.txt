cmake_minimum_required (VERSION 3.16)

project(libVRaster)

message("Build Type: ${CMAKE_BUILD_TYPE}")

add_library(VRaster src/main.cpp)

set_target_properties(VRaster PROPERTIES
    PUBLIC_HEADER include/VRaster.h
)

# OpenAL
find_package(OpenAL CONFIG REQUIRED)
target_link_libraries(VRaster PRIVATE OpenAL::OpenAL)

# libsndfile
find_package(SndFile CONFIG REQUIRED)
target_link_libraries(VRaster PRIVATE SndFile::sndfile)

# GLM
find_package(glm CONFIG REQUIRED)
target_link_libraries(VRaster PRIVATE glm::glm)

# tinygltf
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(VRaster PRIVATE ${TINYGLTF_INCLUDE_DIRS})

# install
install(TARGETS VRaster
    EXPORT VRaster-config
    LIBRARY        DESTINATION lib
    INCLUDES       DESTINATION include
    PUBLIC_HEADER  DESTINATION include/Vraster)

install(EXPORT VRaster-config
    FILE VRaster-config.cmake
    DESTINATION share/cmake/VRaster/
    EXPORT_LINK_INTERFACE_LIBRARIES)
