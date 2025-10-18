#pragma once

#include "CopyDepthTextureToBuffer.hpp"
#include "DescriptorModifier.hpp"
#include "FogInfo.hpp"
#include "RenderResourceModifier.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarCamera.hpp"
#include "StarComputePipeline.hpp"
#include "StarObjectInstance.hpp"
#include "StarShaderInfo.hpp"
#include "core/renderer/RenderingContext.hpp"
#include "FogControlInfo.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"

#include <memory>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

class VolumeRenderer : private star::DescriptorModifier
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

    VolumeRenderer(std::string vdbFilePath, std::shared_ptr<FogInfo> fogControlInfo,
                   const std::shared_ptr<star::StarCamera> camera, const star::ManagerController::RenderResource::Buffer *instanceManagerInfo,
                   std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToColors,
                   std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToDepths,
                   const std::vector<star::Handle> &globalInfoBuffers, const std::vector<star::Handle> &globalLightList,
                   const std::vector<star::Handle> &sceneLightInfoBuffers, const std::array<glm::vec4, 2> &aabbBounds);

    bool isRenderReady(star::core::device::DeviceContext &context);

    void frameUpdate(star::core::device::DeviceContext &context);

    void prepRender(star::core::device::DeviceContext &device, const vk::Extent2D &screensize,
                    const uint8_t &numFramesInFlight);

    void cleanupRender(star::core::device::DeviceContext &device);

    // star::core::renderer::RenderingContext buildRenderingConte    std::vector<star::Handle> fogControlShaderInfo;xt(star::core::device::DeviceContext &context);

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
    std::string m_vdbFilePath;
    std::unique_ptr<star::core::renderer::RenderingContext> m_renderingContext = nullptr;
    bool isReady = false;
    bool isFirstPass = true;
    const star::Handle volumeTexture;
    const star::ManagerController::RenderResource::Buffer *m_instanceManagerInfo = nullptr;
    const std::array<glm::vec4, 2> &aabbBounds;
    const std::shared_ptr<star::StarCamera> camera = nullptr;
    glm::uvec2 workgroupSize = glm::uvec2();
    star::Handle cameraShaderInfo, commandBuffer, vdbInfoSDF, vdbInfoFog, randomValueTexture;
    std::vector<star::Handle> sceneLightInfoBuffers, sceneLightList;
    FogInfoController m_fogController;
    std::unique_ptr<star::StarShaderInfo> SDFShaderInfo, VolumeShaderInfo;
    std::vector<star::Handle> globalInfoBuffers, aabbInfoBuffers;
    std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToColors = nullptr, *offscreenRenderToDepths = nullptr;
    std::unique_ptr<uint32_t> graphicsQueueFamilyIndex, computeQueueFamilyIndex;

    std::vector<std::shared_ptr<star::StarTextures::Texture>> computeWriteToImages =
        std::vector<std::shared_ptr<star::StarTextures::Texture>>();
    std::unique_ptr<vk::PipelineLayout> computePipelineLayout = std::unique_ptr<vk::PipelineLayout>();
    star::Handle marchedPipeline, nanoVDBPipeline_hitBoundingBox, nanoVDBPipeline_surface, linearPipeline, expPipeline;
    std::vector<std::unique_ptr<star::StarBuffers::Buffer>> renderToDepthBuffers =
        std::vector<std::unique_ptr<star::StarBuffers::Buffer>>();

    FogType currentFogType = FogType::marched;

    void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex);

    std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int &numFramesInFlight) override;

    std::unique_ptr<star::StarShaderInfo> buildShaderInfo(star::core::device::DeviceContext &context,
                                                          const uint8_t &numFramesInFlight, const bool &useSDF) const;

    void createDescriptors(star::core::device::DeviceContext &device, const int &numFramesInFlight) override;

    static glm::uvec2 CalculateWorkGroupSize(const vk::Extent2D &screenSize);
};