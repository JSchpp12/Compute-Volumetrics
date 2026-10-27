#pragma once

#include "FogPipelineSet.hpp"
#include "FogType.hpp"

#include <star_common/Handle.hpp>
#include <starlight/wrappers/graphics/StarShaderInfo.hpp>

#include <optional>
#include <vulkan/vulkan.hpp>

namespace render_system::fog
{

/// Every volume pipeline the phase builds, in registration form. Persists for the phase's lifetime; the
/// values a given frame's passes need are resolved into PassPipelineInfo.
struct VolumePipelineSet
{
    FogPipelineSet fog{};
    star::Handle init{};
    star::Handle indirectDispatch{};
    star::Handle transmittance{};
};

/// @brief Runtime check container for the graphics -> compute image acquires
struct ImageReady
{
    bool offscreenColor{false};
    bool offscreenDepth{false};
    bool shadowDepth{false};
    bool volumeOutput{false};
};

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
    vk::Image transmittanceMap{VK_NULL_HANDLE};
    vk::Buffer computeRayAtCutoffDistance{VK_NULL_HANDLE};
    vk::Buffer computeRayDistance{VK_NULL_HANDLE};
    bool transferWasRunLast{false};         // flag set to signal if a dedicated transfer was run last frame
    bool transferWillBeRunThisFrame{false}; // flag set to signal if a dedicated transfer will be run this frame
    bool depthPassWillRun{false};
    // Toggled as the chunks record: whichever chunk records an image's acquire barrier flips its flag.
    ImageReady imageReady{};
};

/// Hot-path view of a single frame: only the pipeline values the passes being recorded will bind, already
/// resolved from their handles, plus that frame's buffers and shader infos.
struct PassPipelineInfo
{
    vk::PipelineLayout layout{VK_NULL_HANDLE};
    vk::PipelineLayout distanceLayout{VK_NULL_HANDLE};
    vk::Pipeline initPipeline{VK_NULL_HANDLE};
    vk::Pipeline indirectDispatchPipeline{VK_NULL_HANDLE};
    vk::Pipeline colorPipeline{VK_NULL_HANDLE};
    vk::Pipeline transmittancePipeline{VK_NULL_HANDLE};
    vk::Pipeline distancePipeline{VK_NULL_HANDLE};
    vk::Buffer indirectDispatchBuffer{VK_NULL_HANDLE};

    // static shared shader info -> such as the camera and light info used by all
    star::StarShaderInfo *staticShaderInfo{nullptr};
    star::StarShaderInfo *colorOnlyShaderInfo{nullptr};
    star::StarShaderInfo *distanceOnlyShaderInfo{nullptr};
    star::StarShaderInfo *transmittanceOnlyShaderInfo{nullptr};
    star::StarShaderInfo *sceneDepthShaderInfo{nullptr};
    star::StarShaderInfo *shadowDepthShaderInfo{nullptr};
    /// Snapshot of the fog type these values were resolved for; the passes are selected from this, not from
    /// the live phase state.
    Fog::Type fogType = Fog::Type::sMarched;
};

} // namespace render_system::fog
