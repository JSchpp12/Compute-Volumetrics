#pragma once

#include "core/renderer/DefaultRenderer.hpp"

class OffscreenRenderer : public star::core::renderer::DefaultRenderer
{
  public:
    OffscreenRenderer(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                      std::vector<std::shared_ptr<star::StarObject>> objects,
                      std::shared_ptr<std::vector<star::Light>> lights, std::shared_ptr<star::StarCamera> camera);

    OffscreenRenderer(OffscreenRenderer &&other) = default;
    OffscreenRenderer &operator=(OffscreenRenderer &&other) = default;

    virtual void recordCommandBuffer(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                     const uint64_t &frameIndex) override;

    void prepRender(star::common::IDeviceContext &context, const uint8_t &numFramesInFlight) override;

    virtual vk::Format getColorAttachmentFormat(star::core::device::DeviceContext &device) const override;

    virtual vk::Format getDepthAttachmentFormat(star::core::device::DeviceContext &device) const override;

  private:
    std::unique_ptr<uint32_t> graphicsQueueFamilyIndex, computeQueueFamilyIndex;
    bool isFirstPass = true;
    uint32_t firstFramePassCounter = 0;

    std::vector<std::shared_ptr<star::StarBuffers::Buffer>> depthInfoStorageBuffers;

    std::vector<star::StarTextures::Texture> createRenderToImages(
        star::core::device::DeviceContext &device, const uint8_t &numFramesInFlight) override;

    std::vector<star::StarTextures::Texture> createRenderToDepthImages(
        star::core::device::DeviceContext &device, const uint8_t &numFramesInFlight) override;

    star::core::device::manager::ManagerCommandBuffer::Request getCommandBufferRequest() override;

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoDepthAttachment(
        const uint8_t &frameInFlightIndex) override;
};