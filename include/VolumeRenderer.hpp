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


class VolumeRenderer :
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

    VolumeRenderer(std::shared_ptr<FogInfo> fogControlInfo,
        const std::shared_ptr<star::StarCamera> camera, const std::vector<star::Handle> &instanceModelInfo,
                std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToColors,
                   std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToDepths,
                   const std::vector<star::Handle> &globalInfoBuffers,
                   const std::vector<star::Handle> &globalLightList,
                   const std::vector<star::Handle> &sceneLightInfoBuffers, 
                   const star::Handle &volumeTexture,
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
  private:
    std::shared_ptr<FogInfo> m_fogControlInfo;
    bool isFirstPass = true;
    const star::Handle volumeTexture;
    const std::vector<star::Handle> &instanceModelInfo;
    const std::array<glm::vec4, 2> &aabbBounds;
    const std::shared_ptr<star::StarCamera> camera = nullptr;
    star::core::device::DeviceID m_deviceID; 
    glm::uvec2 workgroupSize = glm::uvec2();
    star::Handle cameraShaderInfo, commandBuffer; 
    std::vector<star::Handle> fogControlShaderInfo;
    std::vector<star::Handle> sceneLightInfoBuffers, sceneLightList; 
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

    void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex); 

    // Inherited via RenderResourceModifier
    void initResources(star::core::device::DeviceContext &device, const int &numFramesInFlight, const vk::Extent2D &screensize) override;

    void destroyResources(star::core::device::DeviceContext &device) override;

    std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int &numFramesInFlight) override;

    void createDescriptors(star::core::device::DeviceContext &device, const int &numFramesInFlight) override;

    static glm::uvec2 CalculateWorkGroupSize(const vk::Extent2D &screenSize);
};