# VRaster

A simple VR app framework written in c++.

## Prerequisites

- Windows 11 (may also work on Windows 10, but not checked yet.)
- C++ environment, any of the following:
    - MSVC / Microsoft Visual Studio 
    - gcc / GNU make
- CMake (>=3.24)
- vcpkg
- Vulkan SDK
- OpenXR
- OpenXR-Hpp
- OpenAL

## Install

```sh
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake -DCMAKE_INSTALL_PREFIX=(install path)
cmake --build . --config Release
cmake --install .
```
```bat
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DCMAKE_INSTALL_PREFIX=(install path)
cmake --build . --config Release
cmake --install .
```


