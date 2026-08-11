#include "renderer/VolumeShadowRenderPhase.hpp"

#include <starlight/core/Exceptions.hpp>

#include <cassert>
#include <functional>
#include <optional>

VolumeShadowRenderPhase::VolumeShadowRenderPhase(bool enableShadowCasting) : m_shadowCastingEnabled(enableShadowCasting)
{
}

void VolumeShadowRenderPhase::frameUpdate(star::common::IDeviceContext &context)
{
    auto &c = static_cast<star::core::device::DeviceContext &>(context);

    if (isRenderReady(c))
    {
        // TODO: update shadow-dependent data for the current frame.
    }
}

bool VolumeShadowRenderPhase::isRenderReady(star::core::device::DeviceContext &context)
{
    if (isReady)
    {
        return true;
    }

    // TODO: gate on the shadow pipelines / resources becoming ready.
    isReady = true;

    return isReady;
}

void VolumeShadowRenderPhase::recordCommandBuffer(star::StarCommandBuffer &commandBuffer,
                                                  const star::common::FrameTracker &frameTracker,
                                                  const uint64_t &frameIndex)
{
    recordCommands(commandBuffer.buffer(frameTracker.getCurrent().getFrameInFlightIndex()), frameTracker, frameIndex);
}

void VolumeShadowRenderPhase::recordCommands(vk::CommandBuffer &commandBuffer,
                                             const star::common::FrameTracker &frameTracker, const uint64_t &frameIndex)
{
    // TODO: record the volume shadow compute commands for this frame.
}

void VolumeShadowRenderPhase::cleanupRender(star::common::IDeviceContext &context)
{
    auto &c = static_cast<star::core::device::DeviceContext &>(context);

    // TODO: release shadow resources (buffers, pipelines, descriptor layouts).
    (void)c;
}

std::optional<star::core::device::manager::ManagerCommandBuffer::BufferSubmissionOverride> VolumeShadowRenderPhase::
    getSubmissionOverride()
{
    return std::nullopt;
}
