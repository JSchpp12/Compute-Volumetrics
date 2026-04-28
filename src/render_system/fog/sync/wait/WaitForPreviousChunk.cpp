#include "render_system/fog/sync/wait/WaitForPreviousChunk.hpp"

#include "render_system/fog/sync/signal/CalcFromFt.hpp"

render_system::fog::WaitInfo render_system::fog::sync::wait::WaitForPreviousChunk::getWaitInfo() const
{
    assert(m_myChunkOrder > 0 && "This cannot be used as the wait approach for the first chunk in the set"); 

    signal::CalcFromFt calculator(m_myChunkOrder-1, m_totalNumChunks, m_ft);

    auto signalInfo = calculator.getSignalInfo();
    return {.info = {signalInfo}, .count = 1};
}