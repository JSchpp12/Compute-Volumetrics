#pragma once

#include "FogType.hpp"

#include <starlight/wrappers/graphics/StarShaderInfo.hpp>

#include <vulkan/vulkan.hpp>

#include <optional>

namespace render_system::fog
{
struct PassInfo
{
    struct TerrainPassInfo
    {
        vk::Image renderToColor{VK_NULL_HANDLE};
        vk::Image renderToDepth{VK_NULL_HANDLE};
        vk::Image renderToShadowDepth{VK_NULL_HANDLE};
    };
    std::optional<vk::Buffer> globalCameraBuffer{std::nullopt};
    std::optional<vk::Buffer> fogControllerBuffer{std::nullopt};

    TerrainPassInfo terrainPassInfo;
    vk::Image computeWriteToImage{VK_NULL_HANDLE};
    vk::Buffer computeRayAtCutoffDistance{VK_NULL_HANDLE};
    vk::Buffer computeRayDistance{VK_NULL_HANDLE};
    bool transferWasRunLast{false};         // flag set to signal if a dedicated transfer was run last frame
    bool transferWillBeRunThisFrame{false}; // flag set to signal if a dedicated transfer will be run this frame
};

struct PassPipelineInfo
{
    struct PipelineInfo
    {
        vk::PipelineLayout layout{VK_NULL_HANDLE};
        vk::Pipeline pipeline{VK_NULL_HANDLE};
    };

    vk::Pipeline initPipeline{VK_NULL_HANDLE};
    vk::Pipeline indirectDispatchPipeline{VK_NULL_HANDLE};
    vk::Buffer indirectDispatchBuffer{VK_NULL_HANDLE};
    PipelineInfo colorPipe;
    PipelineInfo distancePipe;
    PipelineInfo transmittancePipe;
    // static shared shader info -> such as the camera and light info used by all
    star::StarShaderInfo *staticShaderInfo{nullptr};
    star::StarShaderInfo *colorOnlyShaderInfo{nullptr};
    star::StarShaderInfo *distanceOnlyShaderInfo{nullptr};
    star::StarShaderInfo *transmittanceOnlyShaderInfo{nullptr};
    star::StarShaderInfo *sceneDepthShaderInfo{nullptr};
    star::StarShaderInfo *shadowDepthShaderInfo{nullptr};
    Fog::Type fogType;
};

} // namespace render_system::fog
