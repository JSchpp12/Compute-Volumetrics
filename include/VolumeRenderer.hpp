#pragma once

#include <vma/vk_mem_alloc.h>

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "AABBInfo.hpp"
#include "CommandBufferModifier.hpp"
#include "CopyDepthTextureToBuffer.hpp"
#include "DescriptorModifier.hpp"
#include "FogInfo.hpp"
#include "RenderResourceModifier.hpp"
#include "SampledVolumeTexture.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarCamera.hpp"
#include "StarComputePipeline.hpp"
#include "StarObjectInstance.hpp"
#include "StarShaderInfo.hpp"


class VolumeRenderer : public star::CommandBufferModifier,
                       private star::RenderResourceModifier,
                       private star::DescriptorModifier
{
  public:
    enum FogType
    {
        linear,
        exp,
        marched
    };

    VolumeRenderer(const std::shared_ptr<star::StarCamera> camera, const std::vector<star::Handle> &instanceModelInfo,
                std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToColors,
                   std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToDepths,
                   const std::vector<star::Handle> &globalInfoBuffers,
                   const std::vector<star::Handle> &sceneLightInfoBuffers, const star::Handle &volumeTexture,
                   const std::array<glm::vec4, 2> &aabbBounds);

    virtual ~VolumeRenderer() = default;

    std::vector<std::unique_ptr<star::StarTextures::Texture>> &getRenderToImages()
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
    FogInfo &getFogControlInfo()
    {
        return *this->fogControlInfo;
    }

  private:
    bool isFirstPass = true;
    const star::Handle volumeTexture;
    const std::vector<star::Handle> &instanceModelInfo;
    const std::array<glm::vec4, 2> &aabbBounds;
    const std::shared_ptr<star::StarCamera> camera = nullptr;
    glm::uvec2 workgroupSize = glm::uvec2();
    star::Handle cameraShaderInfo;
    std::vector<star::Handle> fogControlShaderInfo;
    std::vector<star::Handle> sceneLightInfoBuffers = std::vector<star::Handle>();
    std::unique_ptr<star::StarShaderInfo> compShaderInfo = std::unique_ptr<star::StarShaderInfo>();
    std::vector<star::Handle> globalInfoBuffers = std::vector<star::Handle>();
    std::vector<star::Handle> aabbInfoBuffers;
    std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToColors = nullptr;
    std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToDepths = nullptr;
    std::unique_ptr<uint32_t> graphicsQueueFamilyIndex, computeQueueFamilyIndex;

    std::unique_ptr<vk::Extent2D> displaySize = std::unique_ptr<vk::Extent2D>();
    std::vector<std::unique_ptr<star::StarTextures::Texture>> computeWriteToImages =
        std::vector<std::unique_ptr<star::StarTextures::Texture>>();
    std::unique_ptr<vk::PipelineLayout> computePipelineLayout = std::unique_ptr<vk::PipelineLayout>();
    std::unique_ptr<star::StarComputePipeline> marchedPipeline = std::unique_ptr<star::StarComputePipeline>(),
                                               linearPipeline = std::unique_ptr<star::StarComputePipeline>(),
                                               expPipeline = std::unique_ptr<star::StarComputePipeline>();
    std::vector<std::unique_ptr<star::StarBuffers::Buffer>> renderToDepthBuffers =
        std::vector<std::unique_ptr<star::StarBuffers::Buffer>>();

    FogType currentFogType = FogType::marched;
    std::shared_ptr<FogInfo> fogControlInfo =
        std::shared_ptr<FogInfo>(new FogInfo(FogInfo::LinearFogInfo(0.001f, 100.0f), FogInfo::ExpFogInfo(0.5f),
                                             FogInfo::MarchedFogInfo(0.002f, 0.3f, 0.3f, 0.2f, 0.1f, 5.0f)));

    // Inherited via CommandBufferModifier
    void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex) override;

    star::Command_Buffer_Order_Index getCommandBufferOrderIndex() override;

    star::Command_Buffer_Order getCommandBufferOrder() override;

    star::Queue_Type getCommandBufferType() override;

    vk::PipelineStageFlags getWaitStages() override;

    bool getWillBeSubmittedEachFrame() override;

    bool getWillBeRecordedOnce() override;

    // Inherited via RenderResourceModifier
    void initResources(star::StarDevice &device, const int &numFramesInFlight, const vk::Extent2D &screensize) override;

    void destroyResources(star::StarDevice &device) override;

    std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int &numFramesInFlight) override;

    void createDescriptors(star::StarDevice &device, const int &numFramesInFlight) override;

    static glm::uvec2 CalculateWorkGroupSize(const vk::Extent2D &screenSize);
};