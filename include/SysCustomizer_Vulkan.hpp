#pragma once

#include "SysCustomizer.hpp"
#include <vulkan/vulkan.hpp>

struct VulkanGraphicsCustomizer {
    vk::Instance instance;
    vk::Device device;
    vk::PhysicalDevice physicalDevice;
    vk::Queue queue;
    vk::CommandBuffer activeCommandBuf;
};
