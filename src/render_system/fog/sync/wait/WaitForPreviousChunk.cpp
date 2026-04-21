#include "render_system/fog/sync/wait/WaitForPreviousChunk.hpp"

#include "render_system/fog/sync/signal/CalcFromFt.hpp"

render_system::fog::sync::WaitInfo render_system::fog::sync::wait::WaitForPreviousChunk::getWaitInfo() const
{
    signal::CalcFromFt calculator{m_myChunkOrder, m_totalNumChunks, m_ft};

    auto signalInfo = calculator.getSignalInfo();
    signalInfo.value = signalInfo.value - m_totalNumChunks;
    return {.info = {signalInfo}, .count = 1};
}