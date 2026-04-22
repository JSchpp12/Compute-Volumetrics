#pragma once

#include "render_system/fog/sync/SyncInfo.hpp"

#include <star_common/FrameTracker.hpp>

namespace render_system::fog::sync::wait
{
class WaitForFirstChunk
{
    uint16_t m_totalNumChunks{0};
    const star::common::FrameTracker *m_ft{nullptr};

  public:
    WaitForFirstChunk(uint16_t totalNumChunks, const star::common::FrameTracker *ft)
        : m_totalNumChunks{std::move(totalNumChunks)} ,m_ft{ft}
    {
    }

    WaitInfo getWaitInfo() const; 
};
}