#pragma once

#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/core/device/managers/Semaphore.hpp>
#include <starlight/data_structure/dynamic/ThreadSharedObjectPool.hpp>
#include <starlight/wrappers/graphics/StarBuffers/Buffer.hpp>
#include <starlight/wrappers/graphics/StarQueue.hpp>

#include <optional>

namespace service::image_metric_manager
{
struct CopyResources
{
    uint64_t signalValue;
    star::Handle semaphoreRecordHandle;
    const star::core::device::manager::SemaphoreRecord *timelineRecord = nullptr;
    const star::StarBuffers::Buffer *rayDistance = nullptr;
    const star::StarBuffers::Buffer *rayAtCutoff = nullptr;
};

class CopyCmds
{
  public:
    explicit CopyCmds(CopyResources &cpyResources);

    void prepRender(star::core::device::StarDevice &device, star::core::CommandBus &cmdBus, star::common::EventBus &eb,
                    star::core::device::manager::ManagerCommandBuffer &cb, star::core::device::manager::Queue &qm,
                    const star::common::FrameTracker &frameTracker);

    void trigger(star::core::device::manager::ManagerCommandBuffer &cmdManager, star::core::CommandBus &cmdBus,
                 const star::StarBuffers::Buffer &targetRayCutoff, const star::StarBuffers::Buffer &targetRayDistance,
                 star::Handle fogRendererRegistration);

    const star::Handle &getCommandBuffer() const
    {
        return m_cmdBuffer;
    }

  private:
    struct CopyTargetInfo
    {
        struct BufferInfo
        {
            const star::StarBuffers::Buffer *buffer = nullptr;
        };

        star::Handle rendererRegistration;
        BufferInfo rayDistance;
        BufferInfo rayAtCutoffDistance;
    };

    CopyResources &m_cpyResources;
    CopyTargetInfo m_targetInfo;
    star::Handle m_cmdBuffer;
    uint8_t m_transferQueueFamilyIndex = 0;
    uint8_t m_computeQueueFamilyIndex = 0;
    vk::Device m_device = VK_NULL_HANDLE;
    star::StarQueue *m_targetTransferQueue = nullptr;
    const star::core::CommandBus *m_cmdBus{nullptr};

    void recordCommandBuffer(star::StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                             const uint64_t &frameIndex);

    void waitForSemaphoreIfNecessary(const star::common::FrameTracker &frameTracker) const;

    star::Handle registerWithManager(star::core::device::StarDevice &device,
                                     star::core::device::manager::ManagerCommandBuffer &cb,
                                     star::core::device::manager::Queue &qm, star::common::EventBus &eb,
                                     star::core::CommandBus &cmdBus, const star::common::FrameTracker &frameTracker);

    void copyBuffer(star::StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                    const star::StarBuffers::Buffer &src, const star::StarBuffers::Buffer &dst) const;

    vk::Semaphore submitBuffer(star::StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                               std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                               std::vector<vk::Semaphore> dataSemaphores,
                               std::vector<vk::PipelineStageFlags> dataWaitPoints,
                               std::vector<std::optional<uint64_t>> previousSignaledValues);

    star::StarQueue *getTransferQueue(star::common::EventBus &eb, star::core::device::manager::Queue &qm) const;

    std::vector<vk::BufferMemoryBarrier2> getPreBufferBarriers() const;

    std::vector<vk::BufferMemoryBarrier2> getPostBufferBarriers() const;

    void addPreMemoryBarriers(vk::CommandBuffer &cmdBuffer) const;

    void addPostMemoryBarriers(vk::CommandBuffer &cmdBuffer) const;
};
} // namespace service::image_metric_manager