#include "renderer/PreMemoryBarrierContributor.hpp"

void renderer::PreMemoryBarrierContributor::recordPreCommands(const VolumePassInfo &tInfo,
                                                              vk::CommandBuffer cmdBuf,
                                                              const star::common::FrameTracker &ft)
{
    if (std::holds_alternative<PreMemoryBarrierDifferentFamilies>(m_approach))
    {
        std::get<PreMemoryBarrierDifferentFamilies>(m_approach).recordPreCommands(tInfo, cmdBuf, ft); 
    }
}