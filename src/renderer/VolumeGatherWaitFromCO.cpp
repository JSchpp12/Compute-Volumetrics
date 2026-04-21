#include "renderer/VolumeGatherWaitFromCO.hpp"

#include <starlight/command/command_order/GetPassInfo.hpp>
#include <starlight/core/Exceptions.hpp>

#include <cassert>

renderer::VolumeWaitInfo renderer::VolumeGatherWaitFromCO::getWaitInfo() const
{
    assert(m_myRegistration.isInitialized() &&
           "The cmd buffer registration for the owner of this instance of the wait approach is invalid");
    assert(m_cmdBus && "The command bus is invalid");

    auto getCmd = star::command_order::GetPassInfo{m_myRegistration};
    m_cmdBus->submit(getCmd);

    renderer::VolumeWaitInfo wInfo{}; 

    std::vector<vk::SemaphoreSubmitInfo> waitInfo;

    const star::command_order::get_pass_info::GatheredPassInfo &ele = getCmd.getReply().get();
    if (ele.edges != nullptr)
    {
        waitInfo.resize(ele.edges->size());

        for (size_t i{0}; i < ele.edges->size(); i++)
        {
            const auto &edge = ele.edges->at(i);

            vk::Semaphore semaphore{VK_NULL_HANDLE};
            uint64_t signalValue{0};
            if (edge.consumer == m_myRegistration)
            {
                auto nCmd = star::command_order::GetPassInfo{edge.producer};
                m_cmdBus->submit(nCmd);

                semaphore = nCmd.getReply().get().signaledSemaphore;
                signalValue = nCmd.getReply().get().toSignalValue;
            }
            else
            {
                auto nCmd = star::command_order::GetPassInfo{edge.consumer};
                m_cmdBus->submit(nCmd);

                semaphore = nCmd.getReply().get().signaledSemaphore;
                signalValue = nCmd.getReply().get().currentSignalValue;
            }

            wInfo.info[wInfo.count] = vk::SemaphoreSubmitInfo()
                              .setSemaphore(semaphore)
                              .setValue(signalValue)
                              .setStageMask(vk::PipelineStageFlagBits2::eAllCommands);
            wInfo.count++; 
        }
    }
    else
    {
        STAR_THROW("Unable to get binary semaphore from offscreen renderer");
    }

    return wInfo;
}