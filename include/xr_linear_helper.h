#pragma once

#include "xr_linear.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

inline auto toG(const xr::Quaternionf& q) {
    return glm::quat(q.w, q.x, q.y, q.z);
}
inline auto toG(const xr::Vector2f& v) {
    return glm::vec2(v.x, v.y);
}
inline auto toG(const xr::Vector3f& v) {
    return glm::vec3(v.x, v.y, v.z);
}
inline auto toG(const xr::Vector4f& v) {
    return glm::vec4(v.x, v.y, v.z, v.w);
}
inline auto toG(const XrMatrix4x4f& mat) {
    glm::mat4x4 gmat;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            gmat[i][j] = mat.m[i * 4 + j];
    return gmat;
}
