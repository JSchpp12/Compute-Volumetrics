#pragma once

#include "FogControlInfo.hpp"
#include "FogType.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarShaderInfo.hpp"
#include "StarTextures/Texture.hpp"
#include "VisibilityDistanceCompute.hpp"
#include "core/renderer/FrameData.hpp"
#include "core/renderer/RenderingContext.hpp"
#include "render_system/fog/FogDispatcher.hpp"
#include "render_system/fog/PassInfo.hpp"
#include "render_system/fog/commands/Color.hpp"
#include "render_system/fog/struct/ShaderFlags.hpp"
#include "structs/FogInfo.hpp"

#include <starlight/core/renderer/RenderPhase.hpp>

#include <star_common/FrameTracker.hpp>
#include <star_common/Handle.hpp>
#include <star_common/IDeviceContext.hpp>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <vector>

class VolumeRenderPhaseProvider;

/// Runtime half of the volume (compute) renderer. Extends RenderPhase but
/// records a compute pass (no render targets/groups): it holds the fog
/// pipelines, the fog controller, the compute write-images it produces, the
/// timeline semaphores, and the offscreen render-to images it samples. All
/// one-shot setup -- queue-family lookup, DeclarePass, descriptor builder,
/// compute write images/buffers, command-buffer request -- lives on
/// VolumeRenderPhaseProvider::build. Per-frame recording, frameUpdate,
/// cleanup, and the submission override (FogDispatcher submit) live here.
class VolumeRenderPhase : public star::core::renderer::RenderPhase
{
  public:
    friend class VolumeRenderPhaseProvider;
    friend class render_system::fog::commands::Color;

    VolumeRenderPhase() = default;
    virtual ~VolumeRenderPhase() = default;
    VolumeRenderPhase(const VolumeRenderPhase &) = delete;
    VolumeRenderPhase &operator=(const VolumeRenderPhase &) = delete;
    VolumeRenderPhase(VolumeRenderPhase &&) = delete;
    VolumeRenderPhase &operator=(VolumeRenderPhase &&) = delete;

    virtual void frameUpdate(star::common::IDeviceContext &context) override;
    virtual void cleanupRender(star::common::IDeviceContext &context) override;
    virtual void recordCommandBuffer(star::StarCommandBuffer &commandBuffer,
                                     const star::common::FrameTracker &frameTracker,
                                     const uint64_t &frameIndex) override;

    bool isRenderReady(star::core::device::DeviceContext &context);

    uint64_t getTimelineSignalValue(const star::common::FrameTracker &frameTracker) const;

    std::vector<std::shared_ptr<star::StarTextures::Texture>> &getRenderToImages()
    {
        return this->computeWriteToImages;
    }
    const std::vector<std::shared_ptr<star::StarTextures::Texture>> &getRenderToImages() const
    {
        return this->computeWriteToImages;
    }
    void setFogType(Fog::Type type)
    {
        this->currentFogType = std::move(type);
    }
    Fog::Type getFogType() const
    {
        return this->currentFogType;
    }

    const star::StarBuffers::Buffer &getRayDistanceBufferAt(const size_t &index) const
    {
        assert(index < computeRayDistanceBuffers.size() && "Requested index out of range for getRayDistanceBuffers");

        return computeRayDistanceBuffers[index];
    }

    const star::StarBuffers::Buffer &getRayAtCutoffBufferAt(const size_t &index) const
    {
        assert(index < computeRayAtCutoffDistanceBuffers.size());

        return computeRayAtCutoffDistanceBuffers[index];
    }

    void setTransferTriggered(bool value)
    {
        transferTriggeredThisFrame = value;
    }

    void setTransferNeighborHandle(star::Handle handle)
    {
        m_transferNeighborHandle = std::move(handle);
    }

    FogInfo &getFogInfo()
    {
        return m_fogController->getFogInfo();
    }
    const FogInfo &getFogInfo() const
    {
        return m_fogController->getFogInfo();
    }
    void setFogInfo(FogInfo newInfo)
    {
        m_fogController->setFogInfo(std::move(newInfo));
    }
    const std::vector<star::Handle> &getTimelineSemaphores() const
    {
        return m_timelineSemaphores;
    }

    void setShaderFlag(render_system::fog::InitShaderFlags flag, bool state) noexcept;
    void setShaderFlag(render_system::fog::MarchShaderFlags flag, bool state) noexcept;
    bool toggleShaderFlag(render_system::fog::MarchShaderFlags flag) noexcept;

  protected:
    virtual std::optional<star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride>
    getSubmissionOverride() override;

  private:
    render_system::fog::PassPipelineInfo m_pipeInfo;
    star::Handle m_indirectDispatchPipe;
    star::Handle m_initPipe;
    std::shared_ptr<star::core::renderer::FrameData> m_frameData;
    std::shared_ptr<star::core::renderer::FrameData> m_volumeFrameData;
    /// Cached role handle for the shared camera controller (looked up from the
    /// offscreen phase's FrameData). Avoids a per-frame registry lookup.
    star::Handle m_cameraRole{};
    star::core::renderer::RenderPhase *m_offscreenPhase = nullptr;
    star::core::renderer::RenderPhase *m_terrainShadowPhase = nullptr;
    star::core::renderer::RenderingContext m_renderingContext = star::core::renderer::RenderingContext();
    star::Handle cameraShaderInfo, vdbInfoFog, randomValueTexture;
    std::shared_ptr<FogInfoController> m_fogController;
    std::unique_ptr<star::StarShaderInfo> m_staticShaderInfo{nullptr}, m_dynamicShaderInfo{nullptr};
    std::vector<star::Handle> aabbInfoBuffers;
    std::vector<std::shared_ptr<star::StarTextures::Texture>> computeWriteToImages =
        std::vector<std::shared_ptr<star::StarTextures::Texture>>();
    std::vector<star::StarBuffers::Buffer> computeRayDistanceBuffers, computeRayAtCutoffDistanceBuffers;
    star::Handle marchedPipeline, nanoVDBPipeline_hitBoundingBox, nanoVDBPipeline_surface, linearPipeline, expPipeline,
        marchedHomogenousPipeline;
    std::vector<star::Handle> m_timelineSemaphores;
    VisibilityDistanceCompute m_distanceComputer;
    render_system::fog::FogDispatcher m_chunkHandler;
    std::vector<std::shared_ptr<star::StarBuffers::Buffer>> m_activeRayStorage;
    uint32_t transferQueueFamilyIndex{0};
    std::optional<star::Handle> m_transferNeighborHandle{std::nullopt};
    std::unique_ptr<vk::PipelineLayout> computePipelineLayout = std::unique_ptr<vk::PipelineLayout>();
    Fog::Type currentFogType = Fog::Type::sMarched;
    bool m_enableColorDebugCutoff{false};
    bool m_enableShadowMapDebug{false};
    bool isReady = false;
    bool isFirstPass = true;
    bool transferTriggeredThisFrame = false;
    star::core::CommandBus *m_cmdBus{nullptr};
    vk::Device m_device{VK_NULL_HANDLE};

    vk::Semaphore submitBuffer(star::StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                               std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                               std::vector<vk::Semaphore> &dataSemaphores,
                               std::vector<vk::PipelineStageFlags> &dataWaitPoints,
                               std::vector<std::optional<uint64_t>> &previousSignaledValues, star::StarQueue &queue);

    void recordCommands(vk::CommandBuffer &commandBuffer, const star::common::FrameTracker &frameTracker,
                        const uint64_t &frameIndex);

    static std::vector<std::pair<vk::DescriptorType, const uint32_t>> getDescriptorRequests(
        const int &numFramesInFlight);

    static std::vector<star::StarBuffers::Buffer> createComputeWriteToBuffers(
        star::core::device::DeviceContext &context, const vk::Extent2D &screenSize, const size_t &dataTypeSize,
        const std::string &debugName, const size_t &numToCreate);

    void recordDependentDataPipelineBarriers(vk::CommandBuffer &commandBuffer, const uint8_t &frameinFlightIndex,
                                             const uint64_t &frameIndex);

    void gatherDependentExternalDataOrderingInfo(star::core::device::DeviceContext &context,
                                                 const uint8_t &frameInFlightIndex);

    void updateDependentData(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    void updateRenderingContext(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);
};
