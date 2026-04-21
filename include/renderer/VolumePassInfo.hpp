#pragma once

#include <starlight/wrappers/graphics/StarShaderInfo.hpp>

#include <vulkan/vulkan.hpp>

#include <optional>

namespace renderer
{
struct VolumePassInfo
{
    struct TerrainPassInfo
    {
        vk::Image renderToColor{VK_NULL_HANDLE};
        vk::Image renderToDepth{VK_NULL_HANDLE};
    };
    std::optional<vk::Buffer> globalCameraBuffer{std::nullopt};
    std::optional<vk::Buffer> fogControllerBuffer{std::nullopt};

    TerrainPassInfo terrainPassInfo;
    vk::Image computeWriteToImage{VK_NULL_HANDLE};
    vk::Buffer computeRayAtCutoffDistance{VK_NULL_HANDLE};
    vk::Buffer computeRayDistance{VK_NULL_HANDLE};
    bool transferWasRunLast{false}; // flag set to signal if a dedicated transfer was run last frame
    bool transferWillBeRunThisFrame{false}; // flag set to signal if a dedicated transfer will be run this frame
};

struct VolumePassPipelineInfo
{
    star::StarShaderInfo *staticShaderInfo{
        nullptr}; // static shared shader info -> such as the camera and light info used by all
};

} // namespace renderer