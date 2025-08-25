#pragma once

#include "SceneRenderer.hpp"

class OffscreenRenderer : public star::SceneRenderer
{
  public:
    OffscreenRenderer(std::shared_ptr<star::StarScene> scene);

    virtual void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex) override;

    virtual void initResources(star::core::devices::DeviceContext &device, const int &numFramesInFlight,
                               const vk::Extent2D &screensize) override;

    virtual vk::Format getColorAttachmentFormat(star::core::devices::DeviceContext &device) const override; 

    virtual vk::Format getDepthAttachmentFormat(star::core::devices::DeviceContext &device) const override; 

  private:
    std::unique_ptr<uint32_t> graphicsQueueFamilyIndex, computeQueueFamilyIndex;
    bool isFirstPass = true;
    uint32_t firstFramePassCounter = 0;

    std::vector<std::shared_ptr<star::StarBuffers::Buffer>> depthInfoStorageBuffers;

    std::vector<std::unique_ptr<star::StarTextures::Texture>> createRenderToImages(star::core::devices::DeviceContext &device,
                                                                         const int &numFramesInFlight) override;

    std::vector<std::unique_ptr<star::StarTextures::Texture>> createRenderToDepthImages(star::core::devices::DeviceContext &device,
                                                                              const int &numFramesInFlight) override;

    std::vector<std::shared_ptr<star::StarBuffers::Buffer>> createDepthBufferContainers(star::core::devices::DeviceContext &device);
    // Inherited via SceneRenderer
    star::core::devices::managers::ManagerCommandBuffer::Request getCommandBufferRequest() override; 

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoDepthAttachment(
        const int &frameInFlightIndex) override;

    static vk::ImageMemoryBarrier2 createMemoryBarrierPrepForDepthCopy(const vk::Image &depthImage);
};