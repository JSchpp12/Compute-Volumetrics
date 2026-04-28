#pragma once

#include "render_system/fog/struct/SyncInfo.hpp"

#include <star_common/FrameTracker.hpp>

namespace render_system::fog::sync::signal
{
class CalcFromFt
{
    uint16_t m_myChunkOrder{0};
    uint16_t m_totalNumChunks{0};
    const star::common::FrameTracker *m_ft{nullptr};

  public:
    CalcFromFt(uint16_t myChunkOrder, uint16_t totalNumChunks, const star::common::FrameTracker *ft)
        : m_myChunkOrder(std::move(myChunkOrder)), m_totalNumChunks(std::move(totalNumChunks)), m_ft(ft)
    {
    }

    vk::SemaphoreSubmitInfo getSignalInfo() const;
};
} // namespace render_system::fog::sync::signal