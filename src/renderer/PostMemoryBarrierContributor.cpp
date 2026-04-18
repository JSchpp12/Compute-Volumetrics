#include "renderer/PostMemoryBarrierContributor.hpp"

void renderer::PostMemoryBarrierContributor::recordPostCommands(const VolumePassInfo &vInfo, vk::CommandBuffer cmdBuf,
                                                                const star::common::FrameTracker &ft)
{
    if (std::holds_alternative<PostMemoryBarrierDifferentFamilies>(m_approach))
    {
        std::get<PostMemoryBarrierDifferentFamilies>(m_approach).recordPostCommands(vInfo, cmdBuf, ft); 
    }
}