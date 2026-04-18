#include "renderer/PreMemoryBarrierContributor.hpp"

void renderer::PreMemoryBarrierContributor::recordPreCommands(vk::CommandBuffer cmdBuf,
                                                              const star::common::FrameTracker &ft)
{
    std::visit([&](auto &approach) { approach.recordPreCommands(cmdBuf, ft); }, m_approach);
}