#pragma once

#include <optional>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "GraphicsProvider.hpp"
#include "SysCustomizer.hpp"
#include "SysCustomizer_Vulkan.hpp"

namespace Game {

class IVibrationProvider {
public:
	virtual void vibrate(float a) = 0;
};

struct Pose {
	glm::vec3 pos;
	glm::quat ori;
};

struct GameData {
	double dt;

	std::optional<Pose> viewPose;
	std::optional<Pose> stagePose;
	std::optional<Pose> handPoses[2];
	bool trigger[2];

	std::optional<std::reference_wrapper<IVibrationProvider>> handVib[2];
};

void init(IGraphicsProvider& g);

void proc(const GameData& dat);

void draw(IGraphicsProvider& g);

}
