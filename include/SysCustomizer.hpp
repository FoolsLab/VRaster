#pragma once

#include <openxr/openxr.hpp>

struct IGraphicsSysCustomizer {};

struct XrSysCustomizer {
    xr::Instance instance;
    xr::SystemId systemId;
    xr::Session session;
};
