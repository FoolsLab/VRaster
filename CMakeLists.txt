cmake_minimum_required (VERSION 3.16)

project(libVRaster)

message("Build Type: ${CMAKE_BUILD_TYPE}")

add_library(VRaster 
    src/main.cpp
    src/xr_process.cpp
    src/GraphicsManager_Vulkan.cpp
    src/libimpl.cpp
)
target_include_directories(VRaster PRIVATE include)

target_compile_features(VRaster PUBLIC cxx_std_20)
target_compile_definitions(VRaster PUBLIC XR_USE_GRAPHICS_API_VULKAN)

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
target_link_libraries(VRaster PUBLIC glm::glm)

# tinygltf
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(VRaster PRIVATE ${TINYGLTF_INCLUDE_DIRS})

# fmt
find_package(fmt CONFIG REQUIRED)
target_link_libraries(VRaster PRIVATE fmt::fmt)

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(VRaster PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(VRaster PRIVATE ${Vulkan_LIBRARIES})

find_path(VULKAN_HPP_INCLUDE_DIRS "vulkan/vulkan.hpp")
target_include_directories(VRaster PRIVATE ${VULKAN_HPP_INCLUDE_DIRS})

# OpenXR
find_package(OpenXR CONFIG REQUIRED)
target_link_libraries(VRaster PRIVATE OpenXR::headers OpenXR::openxr_loader)

# test
add_executable(VRaster-test-empty src/test/test_empty.cpp)
target_link_libraries(VRaster-test-empty VRaster)
target_include_directories(VRaster-test-empty PRIVATE include)

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
