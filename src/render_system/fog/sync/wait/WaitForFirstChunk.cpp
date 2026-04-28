#include "render_system/fog/sync/wait/WaitForFirstChunk.hpp"

#include "render_system/fog/sync/signal/CalcFromFt.hpp"

render_system::fog::WaitInfo render_system::fog::sync::wait::WaitForFirstChunk::getWaitInfo() const
{
    signal::CalcFromFt calculator{0, m_totalNumChunks, m_ft};

    auto signalInfo = calculator.getSignalInfo();
    return {.info = {signalInfo}, .count = 1};
}