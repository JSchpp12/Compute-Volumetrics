#include "render_system/fog/sync/signal/CalcFromFt.hpp"

vk::SemaphoreSubmitInfo render_system::fog::sync::signal::CalcFromFt::getSignalInfo() const
{
    assert(m_ft != nullptr);

    const uint64_t value = ((m_ft->getCurrent().getNumTimesFrameProcessed() + 1) * m_totalNumChunks) + m_myChunkOrder;

    return vk::SemaphoreSubmitInfo().setValue(value);
}