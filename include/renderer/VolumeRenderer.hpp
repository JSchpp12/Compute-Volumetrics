#pragma once

#include "FogControlInfo.hpp"
#include "FogInfo.hpp"
#include "FogType.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"
#include "OffscreenRenderer.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarCamera.hpp"
#include "StarShaderInfo.hpp"
#include "VisibilityDistanceCompute.hpp"
#include "VolumeDirectoryProcessor.hpp"
#include "core/renderer/RenderingContext.hpp"
#include "render_system/fog/commands/Color.hpp"
#include "render_system/fog/ChunkDispatchGrid.hpp"

#include <star_common/Handle.hpp>

#include <vma/vk_mem_alloc.h>

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

class VolumeRenderer
{
  public:
    friend class render_system::fog::commands::Color;

    VolumeRenderer(star::core::device::DeviceContext &context,
                   std::shared_ptr<star::ManagerController::RenderResource::Buffer> instanceManagerInfo,
                   std::shared_ptr<star::ManagerController::RenderResource::Buffer> instanceNormalInfo,
                   std::shared_ptr<star::ManagerController::RenderResource::Buffer> globalInfoBuffers,
                   std::shared_ptr<star::ManagerController::RenderResource::Buffer> globalLightList,
                   std::shared_ptr<star::ManagerController::RenderResource::Buffer> sceneLightInfoBuffers,
                   OffscreenRenderer *offscreenRenderer, std::string vdbFilePath,
                   const std::shared_ptr<star::StarCamera> camera, const std::array<glm::vec4, 2> &aabbBounds);

    void init(star::core::device::DeviceContext &context);

    bool isRenderReady(const star::core::device::DeviceContext &context);

    void frameUpdate(star::core::device::DeviceContext &context, uint8_t frameInFlightIndex);

    void prepRender(star::core::device::DeviceContext &device, const vk::Extent2D &screensize);

    void cleanupRender(star::core::device::DeviceContext &device);

    void recordCommandBuffer(star::StarCommandBuffer &commandBuffer, const star::common::FrameTracker &frameTracker,
                             const uint64_t &frameIndex);

    void recordCommands(vk::CommandBuffer &commandBuffer, const star::common::FrameTracker &frameTracker,
                        const uint64_t &frameIndex);


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
        if (m_staticShaderInfo != nullptr)
        {
            if (type == Fog::Type::sNanoSurface)
            {
                m_staticShaderInfo->setNewResource(0, 0, 1, star::StarShaderInfo::BufferInfo{vdbInfoSDF});
            }
            else if ((type == Fog::Type::sMarched || type == Fog::Type::sNanoBoundingBox) &&
                     (currentFogType != Fog::Type::sMarched || currentFogType != Fog::Type::sNanoBoundingBox))
            {
                m_staticShaderInfo->setNewResource(0, 0, 1, star::StarShaderInfo::BufferInfo{vdbInfoSDF});
            }
        }

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

    const star::Handle &getCommandBuffer() const
    {
        return m_commandBuffer;
    }
    FogInfo &getFogInfo()
    {
        return m_fogController.getFogInfo();
    }
    const FogInfo &getFogInfo() const
    {
        return m_fogController.getFogInfo();
    }
    void setFogInfo(FogInfo newInfo)
    {
        m_fogController.setFogInfo(std::move(newInfo));
    }
    const std::vector<star::Handle> &getTimelineSemaphores() const
    {
        return m_timelineSemaphores;
    }

  private:
    std::shared_ptr<star::ManagerController::RenderResource::Buffer> m_infoManagerInstanceModel,
        m_infoManagerInstanceNormal, m_infoManagerGlobalCamera, m_infoManagerSceneLightInfo,
        m_infoManagerSceneLightList;
    OffscreenRenderer *m_offscreenRenderer = nullptr;
    std::string m_vdbFilePath;
    star::core::renderer::RenderingContext m_renderingContext = star::core::renderer::RenderingContext();
    const star::Handle volumeTexture;
    const std::array<glm::vec4, 2> &aabbBounds;
    const std::shared_ptr<star::StarCamera> camera = nullptr;
    star::Handle cameraShaderInfo, m_commandBuffer, vdbInfoSDF, vdbInfoFog, randomValueTexture;
    FogInfoController m_fogController;
    std::unique_ptr<star::StarShaderInfo> m_staticShaderInfo{nullptr}, m_dynamicShaderInfo{nullptr};
    std::vector<star::Handle> aabbInfoBuffers;
    uint32_t graphicsQueueFamilyIndex, computeQueueFamilyIndex, transferQueueFamilyIndex;
    std::vector<std::shared_ptr<star::StarTextures::Texture>> computeWriteToImages =
        std::vector<std::shared_ptr<star::StarTextures::Texture>>();
    std::vector<star::StarBuffers::Buffer> computeRayDistanceBuffers, computeRayAtCutoffDistanceBuffers;
    std::unique_ptr<vk::PipelineLayout> computePipelineLayout = std::unique_ptr<vk::PipelineLayout>();
    star::Handle marchedPipeline, nanoVDBPipeline_hitBoundingBox, nanoVDBPipeline_surface, linearPipeline, expPipeline,
        marchedHomogenousPipeline;
    std::vector<std::unique_ptr<star::StarBuffers::Buffer>> renderToDepthBuffers =
        std::vector<std::unique_ptr<star::StarBuffers::Buffer>>();
    std::vector<star::Handle> m_timelineSemaphores;
    VisibilityDistanceCompute m_distanceComputer;
    render_system::fog::ChunkDispatchGrid m_chunkHandler; 

    Fog::Type currentFogType = Fog::Type::sMarched;
    bool isReady = false;
    bool isFirstPass = true;
    bool transferTriggeredThisFrame = false;
    star::core::CommandBus *m_cmdBus{nullptr};
    star::core::device::manager::Semaphore *m_mgrSemaphore{nullptr};
    vk::Device m_device{VK_NULL_HANDLE};



    vk::Semaphore submitBuffer(star::StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                               std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                               std::vector<vk::Semaphore> dataSemaphores,
                               std::vector<vk::PipelineStageFlags> dataWaitPoints,
                               std::vector<std::optional<uint64_t>> previousSignaledValues, star::StarQueue &queue);

    void recordQueueFamilyInfo(star::core::device::DeviceContext &context);

    std::vector<std::pair<vk::DescriptorType, const uint32_t>> getDescriptorRequests(const int &numFramesInFlight);

    void recordDependentDataPipelineBarriers(vk::CommandBuffer &commandBuffer, const uint8_t &frameinFlightIndex,
                                             const uint64_t &frameIndex);

    void gatherDependentExternalDataOrderingInfo(star::core::device::DeviceContext &context,
                                                 const uint8_t &frameInFlightIndex);

    void updateDependentData(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    void updateRenderingContext(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    std::array<vk::BufferMemoryBarrier2, 2> getBufferBarriersFromTransferQueues(
        const star::common::FrameTracker &ft) const;

    std::array<vk::BufferMemoryBarrier2, 2> getBufferBarriersToTransferQueues(
        const star::common::FrameTracker &ft) const;

    std::vector<std::shared_ptr<star::StarTextures::Texture>> createComputeWriteToImages(
        star::core::device::DeviceContext &context, const vk::Extent2D &screenSize, const size_t &numToCreate) const;
    std::vector<star::StarBuffers::Buffer> createComputeWriteToBuffers(star::core::device::DeviceContext &context,
                                                                       const vk::Extent2D &screenSize,
                                                                       const size_t &dataTypeSize,
                                                                       const std::string &debugName,
                                                                       const size_t &numToCreate) const;
};