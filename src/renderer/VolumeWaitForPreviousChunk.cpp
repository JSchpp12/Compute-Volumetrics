#include "renderer/VolumeWaitForPreviousChunk.hpp"

#include "renderer/VolumeSyncCalcFromFt.hpp"

renderer::VolumeWaitInfo renderer::VolumeWaitForPreviousChunk::getWaitInfo() const
{
    VolumeSyncCalcFromFt calculator{m_myChunkOrder, m_totalNumChunks, m_ft};

    auto signalInfo = calculator.getSignalInfo();
    signalInfo.value = signalInfo.value - m_totalNumChunks;
    return {.info = {signalInfo}, .count = 1};
}