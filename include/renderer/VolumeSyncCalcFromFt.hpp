#pragma once

#include "renderer/VolumeSyncInfo.hpp"

#include <star_common/FrameTracker.hpp>

namespace renderer
{
class VolumeSyncCalcFromFt
{
    uint8_t m_myChunkOrder{0};
    uint8_t m_totalNumChunks{0};
    const star::common::FrameTracker *m_ft{nullptr};

  public:
    VolumeSyncCalcFromFt(uint8_t myChunkOrder, uint8_t totalNumChunks, const star::common::FrameTracker *ft)
        : m_myChunkOrder(std::move(myChunkOrder)), m_totalNumChunks(std::move(totalNumChunks)), m_ft(ft)
    {
    }

    vk::SemaphoreSubmitInfo getSignalInfo() const;
};
} // namespace renderer