#include "service/detail/image_metric_manager/CopyCmds.hpp"

#include <starlight/command/command_order/DeclarePass.hpp>
#include <starlight/command/command_order/TriggerPass.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>

namespace image_metric_manager
{
CopyCmds::CopyCmds(CopyResources &cpyResources) : m_cpyResources(cpyResources)
{
}

void CopyCmds::prepRender(star::core::device::StarDevice &device, star::core::CommandBus &cmdBus,
                          star::common::EventBus &eb, star::core::device::manager::ManagerCommandBuffer &cb,
                          star::core::device::manager::Queue &qm, const star::common::FrameTracker &frameTracker)
{
    m_device = device.getVulkanDevice();
    m_cmdBuffer = registerWithManager(device, cb, qm, eb, cmdBus, frameTracker);
    m_targetTransferQueue = getTransferQueue(eb, qm);

    m_cachedTriggerCmdType =
        cmdBus.registerCommandType(star::command_order::trigger_pass::GetTriggerPassCommandTypeName());
}

void CopyCmds::trigger(star::core::device::manager::ManagerCommandBuffer &cmdManager, star::core::CommandBus &cmdBus,
                       const star::StarBuffers::Buffer &targetRayCutoff,
                       const star::StarBuffers::Buffer &targetRayDistance)
{
    assert(m_cmdBuffer.isInitialized() &&
           "CopyCmds instance must be registered with manager before request to record + submit can be made");

    {
        auto tCmd = star::command_order::TriggerPass(m_cachedTriggerCmdType, m_cmdBuffer);
        cmdBus.submit(tCmd);
    }

    m_targetInfo.rayDistance.buffer = &targetRayDistance;
    m_targetInfo.rayAtCutoffDistance.buffer = &targetRayCutoff;

    cmdManager.submitDynamicBuffer(m_cmdBuffer);
}

void CopyCmds::recordCommandBuffer(star::StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                                   const uint64_t &frameIndex)
{
    waitForSemaphoreIfNecessary(frameTracker);

    // get resources from compute queue neighbor

    buffer.begin(frameTracker.getCurrent().getFrameInFlightIndex());

    vk::CommandBuffer &b = buffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex());

    addPreMemoryBarriers(b);

    copyBuffer(buffer, frameTracker, *m_targetInfo.rayDistance.buffer, *m_cpyResources.rayDistance);
    copyBuffer(buffer, frameTracker, *m_targetInfo.rayAtCutoffDistance.buffer, *m_cpyResources.rayAtCutoff);

    addPostMemoryBarriers(b);
    // always give resources back to compute queue neighbor

    b.end();
}

void CopyCmds::copyBuffer(star::StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                          const star::StarBuffers::Buffer &src, const star::StarBuffers::Buffer &dst) const
{
    assert(m_targetInfo.rayDistance.buffer != nullptr);

    auto &b = buffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex());

    b.copyBuffer(src.getVulkanBuffer(), dst.getVulkanBuffer(),
                 vk::BufferCopy().setSize(src.getBufferSize()).setDstOffset(0).setSrcOffset(0));
}

void CopyCmds::waitForSemaphoreIfNecessary(const star::common::FrameTracker &frameTracker) const
{
    // check what the last value of the semaphore was
    assert(m_cpyResources.timelineRecord != nullptr);

    if (m_cpyResources.timelineRecord->timelineValue.value() == frameTracker.getCurrent().getNumTimesFrameProcessed())
    {
        const auto &value = m_cpyResources.timelineRecord->timelineValue.value();
        const auto waitResult = m_device.waitSemaphores(
            vk::SemaphoreWaitInfo().setSemaphores(m_cpyResources.timelineRecord->semaphore).setValues(value),
            UINT64_MAX);

        if (waitResult != vk::Result::eSuccess)
        {
            STAR_THROW("Failed to wait for semaphores");
        }
    }
}

star::Handle CopyCmds::registerWithManager(star::core::device::StarDevice &device,
                                           star::core::device::manager::ManagerCommandBuffer &cb,
                                           star::core::device::manager::Queue &qm, star::common::EventBus &eb,
                                           star::core::CommandBus &cmdBus,
                                           const star::common::FrameTracker &frameTracker)
{
    auto cmdBuff =
        cb.submit(device, frameTracker.getCurrent().getGlobalFrameCounter(),
                  star::core::device::manager::ManagerCommandBuffer::Request{
                      .recordBufferCallback = std::bind(&CopyCmds::recordCommandBuffer, this, std::placeholders::_1,
                                                        std::placeholders::_2, std::placeholders::_3),
                      .order = star::Command_Buffer_Order::end_of_frame,
                      .orderIndex = star::Command_Buffer_Order_Index::first,
                      .type = star::Queue_Type::Ttransfer,
                      .willBeSubmittedEachFrame = false,
                      .recordOnce = false,
                      .overrideBufferSubmissionCallback = std::bind(
                          &CopyCmds::submitBuffer, this, std::placeholders::_1, std::placeholders::_2,
                          std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6)});

    const auto type = cmdBus.registerCommandType(star::command_order::declare_pass::GetDeclarePassCommandTypeName());
    auto *dQueue = star::core::helper::GetEngineDefaultQueue(eb, qm, star::Queue_Type::Ttransfer);
    assert(dQueue != nullptr && "Unable to acquire default transfer queue from engine");

    m_transferQueueFamilyIndex = dQueue->getParentQueueFamilyIndex();
    {
        auto tCmd = star::command_order::DeclarePass(type, cmdBuff, dQueue->getParentQueueFamilyIndex());
        cmdBus.submit(tCmd);
    }

    dQueue = star::core::helper::GetEngineDefaultQueue(eb, qm, star::Queue_Type::Tcompute);
    assert(dQueue != nullptr && "Unable to acquire default compute queue from engine");
    m_computeQueueFamilyIndex = dQueue->getParentQueueFamilyIndex();

    return cmdBuff;
}

vk::Semaphore CopyCmds::submitBuffer(star::StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                                     std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                                     std::vector<vk::Semaphore> dataSemaphores,
                                     std::vector<vk::PipelineStageFlags> dataWaitPoints,
                                     std::vector<std::optional<uint64_t>> previousSignaledValues)
{
    assert(previousCommandBufferSemaphores != nullptr);
    assert(m_targetTransferQueue != nullptr);

    const size_t fIndex = static_cast<size_t>(frameTracker.getCurrent().getFrameInFlightIndex());

    auto waitSemaphores = std::vector<vk::Semaphore>();
    auto waitStages = std::vector<vk::PipelineStageFlags>();
    auto waitSemaphoreValues = std::vector<uint64_t>();

    auto signalSemaphores = std::vector<vk::Semaphore>{m_cpyResources.timelineRecord->semaphore};
    auto signalValues = std::vector<uint64_t>{m_cpyResources.signalValue};

    if (previousCommandBufferSemaphores != nullptr)
    {
        for (size_t i{0}; i < previousCommandBufferSemaphores->size(); i++)
        {
            waitSemaphores.push_back(previousCommandBufferSemaphores->at(i));
            waitSemaphoreValues.push_back(0);
            waitStages.push_back(vk::PipelineStageFlagBits::eTransfer);
        }
    }

    const vk::TimelineSemaphoreSubmitInfo time = vk::TimelineSemaphoreSubmitInfo()
                                                     .setWaitSemaphoreValues(waitSemaphoreValues)
                                                     .setSignalSemaphoreValues(signalValues);
    const vk::SubmitInfo submit =
        vk::SubmitInfo()
            .setCommandBufferCount(1)
            .setPWaitSemaphores(waitSemaphores.data())
            .setWaitSemaphoreCount(waitSemaphores.size())
            .setPCommandBuffers(&buffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()))
            .setPWaitDstStageMask(waitStages.data())
            .setPSignalSemaphores(signalSemaphores.data())
            .setSignalSemaphoreCount(signalSemaphores.size())
            .setPNext(&time);

    m_targetTransferQueue->getVulkanQueue().submit(submit);

    m_cpyResources.timelineRecord->timelineValue = m_cpyResources.signalValue;

    return VK_NULL_HANDLE;
}

star::StarQueue *CopyCmds::getTransferQueue(star::common::EventBus &eb, star::core::device::manager::Queue &qm) const
{
    auto *queue = star::core::helper::GetEngineDefaultQueue(eb, qm, star::Queue_Type::Ttransfer);

    if (queue == nullptr)
    {
        STAR_THROW("Unable to acquire default engine transfer queue");
    }

    return queue;
}

static vk::BufferMemoryBarrier2 MakePreBarrier(uint8_t srcFamilyIndex, uint8_t dstFamilyIndex, vk::Buffer buffer)
{
    return vk::BufferMemoryBarrier2()
        .setSize(vk::WholeSize)
        .setBuffer(std::move(buffer))
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
        .setSrcAccessMask(vk::AccessFlagBits2::eNone)
        .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setDstAccessMask(vk::AccessFlagBits2::eTransferRead)
        .setSrcQueueFamilyIndex(std::move(srcFamilyIndex))
        .setDstQueueFamilyIndex(std::move(dstFamilyIndex));
}

static vk::BufferMemoryBarrier2 MakePreBarrierSameQueue(vk::Buffer buffer)
{
    return vk::BufferMemoryBarrier2()
        .setSize(vk::WholeSize)
        .setBuffer(std::move(buffer))
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setDstAccessMask(vk::AccessFlagBits2::eTransferRead);
}

std::vector<vk::BufferMemoryBarrier2> CopyCmds::getPreBufferBarriers() const
{
    assert(m_cpyResources.rayDistance != nullptr && m_cpyResources.rayAtCutoff != nullptr &&
           "All buffer must be defined in m_cpyResources before barriers can be made");

    const bool diffFamilies = m_transferQueueFamilyIndex != m_computeQueueFamilyIndex;

    if (diffFamilies)
    {
        return {MakePreBarrier(m_computeQueueFamilyIndex, m_transferQueueFamilyIndex,
                               m_targetInfo.rayDistance.buffer->getVulkanBuffer()),
                MakePreBarrier(m_computeQueueFamilyIndex, m_transferQueueFamilyIndex,
                               m_targetInfo.rayAtCutoffDistance.buffer->getVulkanBuffer())};
    }
    else
    {
        return {MakePreBarrierSameQueue(m_targetInfo.rayDistance.buffer->getVulkanBuffer()),
                MakePreBarrierSameQueue(m_targetInfo.rayAtCutoffDistance.buffer->getVulkanBuffer())};
    }
}

void CopyCmds::addPreMemoryBarriers(vk::CommandBuffer &cmdBuffer) const
{
    const auto buffBarriers = getPreBufferBarriers();

    cmdBuffer.pipelineBarrier2(vk::DependencyInfo().setBufferMemoryBarriers(buffBarriers));
}

static vk::BufferMemoryBarrier2 MakePostBarrier(const uint8_t srcFamilyIndex, uint8_t dstFamilyIndex, vk::Buffer buffer)
{
    return vk::BufferMemoryBarrier2()
        .setSize(vk::WholeSize)
        .setBuffer(std::move(buffer))
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setSrcAccessMask(vk::AccessFlagBits2::eTransferRead)
        .setDstStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe)
        .setDstAccessMask(vk::AccessFlagBits2::eNone)
        .setSrcQueueFamilyIndex(std::move(srcFamilyIndex))
        .setDstQueueFamilyIndex(std::move(dstFamilyIndex));
}

std::vector<vk::BufferMemoryBarrier2> CopyCmds::getPostBufferBarriers() const
{
    assert(m_cpyResources.rayDistance != nullptr && m_cpyResources.rayAtCutoff != nullptr &&
           "All buffer must be defined in m_cpyResources before barriers can be made");

    const bool diffFamilies = m_transferQueueFamilyIndex != m_computeQueueFamilyIndex;

    if (diffFamilies)
    {
        return {MakePostBarrier(m_transferQueueFamilyIndex, m_computeQueueFamilyIndex,
                                m_targetInfo.rayAtCutoffDistance.buffer->getVulkanBuffer()),
                MakePostBarrier(m_transferQueueFamilyIndex, m_computeQueueFamilyIndex,
                                m_targetInfo.rayDistance.buffer->getVulkanBuffer())};
    }
    else
    {
        return {};
    }
}

void CopyCmds::addPostMemoryBarriers(vk::CommandBuffer &cmdBuffer) const
{
    const auto buffBarrier = getPostBufferBarriers();

    cmdBuffer.pipelineBarrier2(vk::DependencyInfo().setBufferMemoryBarriers(buffBarrier));
}
} // namespace image_metric_manager