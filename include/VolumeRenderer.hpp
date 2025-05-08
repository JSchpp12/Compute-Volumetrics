#pragma once

#include <vma/vk_mem_alloc.h>

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "AABBInfo.hpp"
#include "CommandBufferModifier.hpp"
#include "CopyDepthTextureToBuffer.hpp"
#include "DescriptorModifier.hpp"
#include "FileTexture.hpp"
#include "RenderResourceModifier.hpp"
#include "SampledVolumeTexture.hpp"
#include "StarBuffer.hpp"
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

    VolumeRenderer(star::StarCamera &camera, const std::vector<star::Handle> &instanceModelInfo,
                   const std::vector<star::Handle> &instanceNormalInfo,
                   std::vector<std::unique_ptr<star::StarTexture>> *offscreenRenderToColors,
                   std::vector<std::unique_ptr<star::StarTexture>> *offscreenRenderToDepths,
                   const std::vector<star::Handle> &globalInfoBuffers,
                   const std::vector<star::Handle> &sceneLightInfoBuffers, const star::Handle &volumeTexture,
                   const std::array<glm::vec4, 2> &aabbBounds);

    ~VolumeRenderer() = default;

    std::vector<std::unique_ptr<star::StarTexture>> *getRenderToImages()
    {
        return &this->computeWriteToImages;
    }

    void setFogType(const FogType &type)
    {
        this->currentFogType = type;
    }
    const FogType &getFogType()
    {
        return this->currentFogType;
    }
    void setFogFarDistance(const float &newFogFarDistance)
    {
        this->fogFarDist = newFogFarDistance;
    }
    const float &getFogFarDistance()
    {
        return this->fogFarDist;
    }
    void setFogNearDistance(const float &newFogNearDistance)
    {
        this->fogNearDist = newFogNearDistance;
    }
    const float &getFogNearDistance()
    {
        return this->fogNearDist;
    }

  private:
    const star::Handle volumeTexture;
    const std::vector<star::Handle> &instanceModelInfo;
    const std::vector<star::Handle> &instanceNormalInfo;
    const std::array<glm::vec4, 2> &aabbBounds;
    const star::StarCamera &camera;
    star::Handle cameraShaderInfo;
    std::vector<star::Handle> fogControlShaderInfo;
    std::vector<star::Handle> sceneLightInfoBuffers = std::vector<star::Handle>();
    std::unique_ptr<star::StarShaderInfo> compShaderInfo = std::unique_ptr<star::StarShaderInfo>();
    std::vector<star::Handle> globalInfoBuffers = std::vector<star::Handle>();
    std::vector<star::Handle> aabbInfoBuffers;
    std::vector<std::unique_ptr<star::StarTexture>> *offscreenRenderToColors = nullptr;
    std::vector<std::unique_ptr<star::StarTexture>> *offscreenRenderToDepths = nullptr;

    std::unique_ptr<vk::Extent2D> displaySize = std::unique_ptr<vk::Extent2D>();
    std::vector<std::unique_ptr<star::StarTexture>> computeWriteToImages =
        std::vector<std::unique_ptr<star::StarTexture>>();
    std::unique_ptr<vk::PipelineLayout> computePipelineLayout = std::unique_ptr<vk::PipelineLayout>();
    std::unique_ptr<star::StarComputePipeline> marchedPipeline,
        linearPipeline = std::unique_ptr<star::StarComputePipeline>();
    std::vector<std::unique_ptr<star::StarBuffer>> renderToDepthBuffers =
        std::vector<std::unique_ptr<star::StarBuffer>>();

    FogType currentFogType = FogType::marched;
    float fogNearDist = 0.001f, fogFarDist = 100.0f;

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
};