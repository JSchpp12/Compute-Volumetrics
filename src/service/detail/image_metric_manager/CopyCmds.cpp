#include "service/detail/image_metric_manager/CopyCmds.hpp"

#include <starlight/command/command_order/DeclarePass.hpp>
#include <starlight/command/command_order/GetPassInfo.hpp>
#include <starlight/command/command_order/TriggerPass.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>

namespace service::image_metric_manager
{
CopyCmds::CopyCmds(CopySrcResources &cpySrcResources, SynchronizedCopyResourcesInfo &cpyDstResources)
    : m_cpySrcResources(cpySrcResources), m_cpyDstResources(cpyDstResources)
{
}

void CopyCmds::prepRender(star::core::device::StarDevice &device, star::core::CommandBus &cmdBus,
                          star::common::EventBus &eb, star::core::device::manager::ManagerCommandBuffer &cb,
                          star::core::device::manager::Queue &qm, const star::common::FrameTracker &frameTracker)
{
    m_cmdBus = &cmdBus;
    m_device = device.getVulkanDevice();
    m_cmdBuffer = registerWithManager(device, cb, qm, eb, cmdBus, frameTracker);
    m_targetTransferQueue = getTransferQueue(eb, qm);
}

void CopyCmds::trigger(star::core::device::manager::ManagerCommandBuffer &cmdManager, star::core::CommandBus &cmdBus)
{
    assert(m_cmdBuffer.isInitialized() &&
           "CopyCmds instance must be registered with manager before request to record + submit can be made");

    cmdBus.submit(star::command_order::TriggerPass()
                      .setPass(m_cmdBuffer)
                      .setTimelineSemaphore(m_cpyDstResources.timelineRecordHandle)
                      .setSignalValue(m_cpyDstResources.signalValue));

    cmdManager.submitDynamicBuffer(m_cmdBuffer);
}

void CopyCmds::recordCommandBuffer(star::StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                                   const uint64_t &frameIndex)
{
    waitForSemaphoreIfNecessary(frameTracker);

    buffer.begin(frameTracker.getCurrent().getFrameInFlightIndex());

    vk::CommandBuffer &b = buffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex());
    addPreMemoryBarriers(b);
    copyBuffer(buffer, frameTracker, m_cpySrcResources.rayDistance, *m_cpyDstResources.cpyDst.rayDistanceBuffer);
    copyBuffer(buffer, frameTracker, m_cpySrcResources.rayAtCutoffDist,
               *m_cpyDstResources.cpyDst.rayAtCutoffDistBuffer);
    addPostMemoryBarriers(b);

    b.end();
}

void CopyCmds::copyBuffer(star::StarCommandBuffer &buffer, const star::common::FrameTracker &frameTracker,
                          const star::StarBuffers::Buffer &src, const star::StarBuffers::Buffer &dst) const
{
    auto &b = buffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex());

    b.copyBuffer(src.getVulkanBuffer(), dst.getVulkanBuffer(),
                 vk::BufferCopy().setSize(src.getBufferSize()).setDstOffset(0).setSrcOffset(0));
}

void CopyCmds::waitForSemaphoreIfNecessary(const star::common::FrameTracker &frameTracker) const
{
    // check what the last value of the semaphore was
    assert(m_cpyDstResources.timelineRecord != nullptr);

    if (m_cpyDstResources.timelineRecord->timelineValue.value() ==
        frameTracker.getCurrent().getNumTimesFrameProcessed())
    {
        const auto &value = m_cpyDstResources.timelineRecord->timelineValue.value();
        const auto waitResult = m_device.waitSemaphores(
            vk::SemaphoreWaitInfo().setSemaphores(m_cpyDstResources.timelineRecord->semaphore).setValues(value),
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
                      .orderIndex = star::Command_Buffer_Order_Index::fifth,
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
    assert(m_targetTransferQueue != nullptr);

    vk::Semaphore volumeSignaledSemaphore{VK_NULL_HANDLE};
    uint64_t volumeSignaledSemaphoreValue{0};
    {
        auto gather = star::command_order::GetPassInfo{m_cpySrcResources.registration};
        m_cmdBus->submit(gather);
        volumeSignaledSemaphore = std::move(gather.getReply().get().signaledSemaphore);
        volumeSignaledSemaphoreValue = std::move(gather.getReply().get().toSignalValue);
    }

    const vk::SemaphoreSubmitInfo waitInfo[1]{
        vk::SemaphoreSubmitInfo().setSemaphore(volumeSignaledSemaphore).setValue(volumeSignaledSemaphoreValue)};

    const vk::SemaphoreSubmitInfo signalInfo[1]{vk::SemaphoreSubmitInfo()
                                                    .setSemaphore(m_cpyDstResources.timelineRecord->semaphore)
                                                    .setValue(m_cpyDstResources.signalValue)};
    const auto cbInfo = vk::CommandBufferSubmitInfo().setCommandBuffer(
        buffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()));

    const auto submitInfo =
        vk::SubmitInfo2().setWaitSemaphoreInfos(waitInfo).setCommandBufferInfos(cbInfo).setSignalSemaphoreInfos(
            signalInfo);

    m_targetTransferQueue->getVulkanQueue().submit2(submitInfo);

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
        .setOffset(0)
        .setBuffer(std::move(buffer))
        .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
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
        .setOffset(0)
        .setBuffer(std::move(buffer))
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setDstAccessMask(vk::AccessFlagBits2::eTransferRead);
}

std::vector<vk::BufferMemoryBarrier2> CopyCmds::getPreBufferBarriers() const
{
    assert(m_cpyDstResources.cpyDst.rayDistanceBuffer != nullptr &&
           m_cpyDstResources.cpyDst.rayAtCutoffDistBuffer != nullptr &&
           "All buffer must be defined in m_cpyDstResources before barriers can be made");

    const bool diffFamilies = m_transferQueueFamilyIndex != m_computeQueueFamilyIndex;
    if (diffFamilies)
    {
        return {MakePreBarrier(m_computeQueueFamilyIndex, m_transferQueueFamilyIndex,
                               m_cpySrcResources.rayDistance.getVulkanBuffer()),
                MakePreBarrier(m_computeQueueFamilyIndex, m_transferQueueFamilyIndex,
                               m_cpySrcResources.rayAtCutoffDist.getVulkanBuffer())};
    }
    else
    {
        return {MakePreBarrierSameQueue(m_cpySrcResources.rayDistance.getVulkanBuffer()),
                MakePreBarrierSameQueue(m_cpySrcResources.rayAtCutoffDist.getVulkanBuffer())};
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
        .setOffset(0)
        .setBuffer(std::move(buffer))
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setSrcAccessMask(vk::AccessFlagBits2::eTransferRead)
        .setDstStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe)
        .setDstAccessMask(vk::AccessFlagBits2::eNone)
        .setSrcQueueFamilyIndex(std::move(srcFamilyIndex))
        .setDstQueueFamilyIndex(std::move(dstFamilyIndex));
}

static vk::BufferMemoryBarrier2 MakeBarrierForTransfer(vk::Buffer buffer)
{
    return vk::BufferMemoryBarrier2()
        .setSize(vk::WholeSize)
        .setOffset(0)
        .setBuffer(std::move(buffer))
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eHost)
        .setDstAccessMask(vk::AccessFlagBits2::eHostRead);
}

std::vector<vk::BufferMemoryBarrier2> CopyCmds::getPostBufferBarriers() const
{
    assert(m_cpyDstResources.cpyDst.rayDistanceBuffer != nullptr &&
           m_cpyDstResources.cpyDst.rayAtCutoffDistBuffer != nullptr &&
           "All buffer must be defined in m_cpyDstResources before barriers can be made");

    const bool diffFamilies = m_transferQueueFamilyIndex != m_computeQueueFamilyIndex;

    std::vector<vk::BufferMemoryBarrier2> barriers = std::vector<vk::BufferMemoryBarrier2>(4);

    barriers[0] = MakeBarrierForTransfer(m_cpyDstResources.cpyDst.rayDistanceBuffer->getVulkanBuffer());
    barriers[1] = MakeBarrierForTransfer(m_cpyDstResources.cpyDst.rayAtCutoffDistBuffer->getVulkanBuffer());
    if (diffFamilies)
    {
        barriers[2] = MakePostBarrier(m_transferQueueFamilyIndex, m_computeQueueFamilyIndex,
                                      m_cpySrcResources.rayAtCutoffDist.getVulkanBuffer());
        barriers[3] = MakePostBarrier(m_transferQueueFamilyIndex, m_computeQueueFamilyIndex,
                                      m_cpySrcResources.rayDistance.getVulkanBuffer());
    }
    else
    {
        return {};
    }

    return barriers;
}

void CopyCmds::addPostMemoryBarriers(vk::CommandBuffer &cmdBuffer) const
{
    const auto buffBarrier = getPostBufferBarriers();

    cmdBuffer.pipelineBarrier2(vk::DependencyInfo().setBufferMemoryBarriers(buffBarrier));
}
} // namespace service::image_metric_manager