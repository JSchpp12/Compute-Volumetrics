#include "renderer/PostMemoryBarrierContributor.hpp"

void renderer::PostMemoryBarrierContributor::recordPostCommands(vk::CommandBuffer cmdBuf,
                                                                const star::common::FrameTracker &ft)
{
    std::visit([&](auto &approach) { approach.recordPostCommands(cmdBuf, ft); }, m_approach);
}