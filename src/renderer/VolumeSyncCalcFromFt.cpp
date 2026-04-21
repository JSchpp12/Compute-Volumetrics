#include "renderer/VolumeSyncCalcFromFt.hpp"

vk::SemaphoreSubmitInfo renderer::VolumeSyncCalcFromFt::getSignalInfo() const
{
    assert(m_ft != nullptr);

    const uint64_t value = ((m_ft->getCurrent().getNumTimesFrameProcessed() + 1) * m_totalNumChunks) + m_myChunkOrder;

    return vk::SemaphoreSubmitInfo().setValue(value);
}