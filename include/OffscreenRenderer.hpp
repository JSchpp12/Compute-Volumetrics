#pragma once

#include <starlight/core/renderer/DefaultRenderer.hpp>

class OffscreenRenderer : public star::core::renderer::DefaultRenderer
{
  public:
    OffscreenRenderer(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                      std::vector<std::shared_ptr<star::StarObject>> objects,
                      std::shared_ptr<std::vector<star::Light>> lights, std::shared_ptr<star::StarCamera> camera);

    OffscreenRenderer(OffscreenRenderer &&other) = default;
    OffscreenRenderer(const OffscreenRenderer &) = delete;
    OffscreenRenderer &operator=(OffscreenRenderer &&other) = default;
    OffscreenRenderer &operator=(const OffscreenRenderer &) = delete;
    virtual ~OffscreenRenderer() = default;

    virtual void recordPreRenderPassCommands(vk::CommandBuffer &buffer, const star::common::FrameTracker &ft) override;

    virtual void recordPostRenderingCalls(vk::CommandBuffer &buffer, const star::common::FrameTracker &ft) override;

    void prepRender(star::common::IDeviceContext &context) override;

    virtual vk::Format getColorAttachmentFormat(star::core::device::DeviceContext &device) const override;

    virtual vk::Format getDepthAttachmentFormat(star::core::device::DeviceContext &device) const override;

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoColorAttachment(
        const star::common::FrameTracker &frameTracker) override;

    virtual void recordCommandBuffer(star::StarCommandBuffer &commandBuffer,
                                     const star::common::FrameTracker &frameInFlightIndex,
                                     const uint64_t &frameIndex) override;

    const std::vector<star::Handle> getTimelineSemaphroes() const
    {
        return m_timelineSemaphores;
    }

  private:
    uint32_t graphicsQueueFamilyIndex = 0;
    uint32_t computeQueueFamilyIndex = 0;
    uint32_t firstFramePassCounter = 0;
    bool isFirstPass = true;
    std::vector<std::shared_ptr<star::StarBuffers::Buffer>> depthInfoStorageBuffers;
    std::vector<star::Handle> m_timelineSemaphores;

    vk::Device m_device{VK_NULL_HANDLE};
    const star::core::CommandBus *m_cmdBus{nullptr};

    std::vector<star::StarTextures::Texture> createRenderToImages(star::core::device::DeviceContext &device,
                                                                  const uint8_t &numFramesInFlight) override;

    std::vector<star::StarTextures::Texture> createRenderToDepthImages(star::core::device::DeviceContext &device,
                                                                       const uint8_t &numFramesInFlight) override;

    star::core::device::manager::ManagerCommandBuffer::Request getCommandBufferRequest() override;

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoDepthAttachment(
        const star::common::FrameTracker &frameTracker) override;

    vk::Semaphore submitBuffer(star::StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                               std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                               std::vector<vk::Semaphore> dataSemaphores,
                               std::vector<vk::PipelineStageFlags> dataWaitPoints,
                               std::vector<std::optional<uint64_t>> previousSignaledValues, star::StarQueue &queue);

    void waitForSemaphore(const star::common::FrameTracker &ft) const;
};