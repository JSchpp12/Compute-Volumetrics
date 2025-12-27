#pragma once

#include "CopyDepthTextureToBuffer.hpp"
#include "FogControlInfo.hpp"
#include "FogInfo.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarCamera.hpp"
#include "StarComputePipeline.hpp"
#include "StarObjectInstance.hpp"
#include "StarShaderInfo.hpp"
#include "VolumeRendererCreateDescriptorsPolicy.hpp"
#include "core/renderer/RenderingContext.hpp"
#include "OffscreenRenderer.hpp"

#include <star_common/Handle.hpp>

#include <memory>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

class VolumeRenderer
{
  public:
    enum FogType
    {
        linear,
        exp,
        marched,
        nano_boundingBox,
        nano_surface
    };

    VolumeRenderer(std::shared_ptr<star::ManagerController::RenderResource::Buffer> instanceManagerInfo,
                   std::shared_ptr<star::ManagerController::RenderResource::Buffer> instanceNormalInfo,
                   std::shared_ptr<star::ManagerController::RenderResource::Buffer> globalInfoBuffers,
                   std::shared_ptr<star::ManagerController::RenderResource::Buffer> globalLightList,
                   std::shared_ptr<star::ManagerController::RenderResource::Buffer> sceneLightInfoBuffers,
                   OffscreenRenderer *offscreenRenderer,
                   std::string vdbFilePath, std::shared_ptr<FogInfo> fogControlInfo,
                   const std::shared_ptr<star::StarCamera> camera, const std::array<glm::vec4, 2> &aabbBounds);

    void init(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight);

    bool isRenderReady(star::core::device::DeviceContext &context);

    void frameUpdate(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    void prepRender(star::core::device::DeviceContext &device, const vk::Extent2D &screensize,
                    const uint8_t &numFramesInFlight);

    void cleanupRender(star::core::device::DeviceContext &device);

    void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const star::common::FrameTracker &frameTracker,
                             const uint64_t &frameIndex);

    std::vector<std::shared_ptr<star::StarTextures::Texture>> &getRenderToImages()
    {
        return this->computeWriteToImages;
    }
    void setFogType(const FogType &type)
    {
        this->currentFogType = type;
    }
    const FogType &getFogType()
    {
        return this->currentFogType;
    }

  private:
    std::shared_ptr<star::ManagerController::RenderResource::Buffer> m_infoManagerInstanceModel,
        m_infoManagerInstanceNormal, m_infoManagerGlobalCamera, m_infoManagerSceneLightInfo,
        m_infoManagerSceneLightList;
    OffscreenRenderer *m_offscreenRenderer = nullptr;
    std::string m_vdbFilePath;
    star::core::renderer::RenderingContext m_renderingContext = star::core::renderer::RenderingContext();
    bool isReady = false;
    bool isFirstPass = true;
    const star::Handle volumeTexture;
    const std::array<glm::vec4, 2> &aabbBounds;
    const std::shared_ptr<star::StarCamera> camera = nullptr;
    glm::uvec2 workgroupSize = glm::uvec2();
    star::Handle cameraShaderInfo, commandBuffer, vdbInfoSDF, vdbInfoFog, randomValueTexture;
    FogInfoController m_fogController;
    std::unique_ptr<star::StarShaderInfo> SDFShaderInfo, VolumeShaderInfo;
    std::vector<star::Handle> aabbInfoBuffers;
    std::unique_ptr<uint32_t> graphicsQueueFamilyIndex, computeQueueFamilyIndex;

    std::vector<std::shared_ptr<star::StarTextures::Texture>> computeWriteToImages =
        std::vector<std::shared_ptr<star::StarTextures::Texture>>();
    std::unique_ptr<vk::PipelineLayout> computePipelineLayout = std::unique_ptr<vk::PipelineLayout>();
    star::Handle marchedPipeline, nanoVDBPipeline_hitBoundingBox, nanoVDBPipeline_surface, linearPipeline, expPipeline;
    std::vector<std::unique_ptr<star::StarBuffers::Buffer>> renderToDepthBuffers =
        std::vector<std::unique_ptr<star::StarBuffers::Buffer>>();

    FogType currentFogType = FogType::marched;

    void registerListenForEngineInitDone(star::common::EventBus &eventBus);

    std::vector<std::pair<vk::DescriptorType, const uint32_t>> getDescriptorRequests(const int &numFramesInFlight);

    std::unique_ptr<star::StarShaderInfo> buildShaderInfo(const star::Handle &deviceID,
                                                          star::core::device::StarDevice &device,
                                                          star::core::device::manager::DescriptorPool &poolManager,
                                                          star::ManagerRenderResource &resourceManager,
                                                          const uint8_t &numFramesInFlight, const bool &useSDF) const;

    void createDescriptors(const star::Handle &deviceID, star::core::device::StarDevice &device,
                           star::core::device::manager::DescriptorPool &poolManager,
                           star::ManagerRenderResource &resourceManager, const uint8_t &numFramesInFlight);

    void recordDependentDataPipelineBarriers(vk::CommandBuffer &commandBuffer, const uint8_t &frameinFlightIndex,
                                             const uint64_t &frameIndex);

    void gatherDependentExternalDataOrderingInfo(star::core::device::DeviceContext &context,
                                                 const uint8_t &frameInFlightIndex);

    void updateDependentData(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    void updateRenderingContext(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex);

    static glm::uvec2 CalculateWorkGroupSize(const vk::Extent2D &screenSize);
};