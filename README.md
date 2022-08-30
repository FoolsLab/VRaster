# VRaster

A simple VR app framework written in c++.

## Prerequisites

- Windows 11 (may also work on Windows 10, Ubuntu, but unconfirmed.)
- C++ environment, any of the following:
    - MSVC / Microsoft Visual Studio 
    - gcc / GNU make
- CMake (>=3.24, recommended)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
    - `VULKAN_SDK` environment variable must be set.
- [vcpkg](https://github.com/microsoft/vcpkg)

### vcpkg packages

These packages are required to install.

- `libsndfile`
- `openal-soft`
- `glm`
- `tinygltf`
- `vulkan`
- `vulkan-hpp`
- `openxr-loader[vulkan]`

## Install

```sh
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=(vcpkg root path)/scripts/buildsystems/vcpkg.cmake -DCMAKE_INSTALL_PREFIX=(install path)
cmake --build . --config Release
cmake --install .
```
