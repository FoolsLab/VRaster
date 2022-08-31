# VRaster

A simple VR app framework written in c++ (WIP).

## Prerequisites

- Windows 11 (This may also work on Windows 10, Ubuntu, but unconfirmed.)
- C++ environment, any of the following:
    - MSVC / Microsoft Visual Studio 2022
    - gcc(>=10) / GNU make
- CMake (>=3.16, recommended)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
    - `VULKAN_SDK` environment variable must be set.
- [vcpkg](https://github.com/microsoft/vcpkg)

### vcpkg packages

These packages are required to install.

- `openal-soft`
- `freealut`
- `glm`
- `tinygltf`
- `fmt`
- `vulkan`
- `vulkan-hpp`
- `openxr-loader[vulkan]`

**CAUTION**: Latest freealut header has a problem and this causes compile errors.
We recommend to fix `AL/alut.h` installed by vcpkg manually.
before:
```cpp
#include <alc.h>
#include <al.h>
```
after:
```cpp
#include "alc.h"
#include "al.h"
```

## Install

```sh
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=(vcpkg root path)/scripts/buildsystems/vcpkg.cmake -DCMAKE_INSTALL_PREFIX=(install path)
cmake --build . --config Release
cmake --install .
```

## How to use

### CMakeLists.txt

```cmake
find_package(VRaster REQUIRED)
target_link_libraries(main VRaster)
```

### C++ Code

```cpp
#include <VRaster.hpp>

void init(IGraphicsProvider& g) {
    // initializing code
}

void proc(const GameData& dat){
    // main-loop processing code
}

void draw(IGraphicsProvider& g) {
    // main-loop drawing code
}
```
