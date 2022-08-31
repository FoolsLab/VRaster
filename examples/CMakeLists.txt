cmake_minimum_required(VERSION 3.16)

project(VRaster-examples)

find_package(VRaster REQUIRED)

find_package(OpenAL CONFIG REQUIRED)
find_package(FreeALUT CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
find_package(fmt CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_path(VULKAN_HPP_INCLUDE_DIRS "vulkan/vulkan.hpp")
find_package(OpenXR CONFIG REQUIRED)

add_executable(simple-box simple-box/main.cpp)
target_link_libraries(simple-box VRaster)

add_executable(simple-stg simple-stg/main.cpp)
target_link_libraries(simple-stg VRaster)
