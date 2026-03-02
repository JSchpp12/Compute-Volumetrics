#include "service/detail/simulation_controller/FogEnabler.hpp"

#include <cassert> 

namespace service::simulation_controller
{

void FogEnabler::setEnabled(Fog::Type type)
{
    assert(static_cast<size_t>(type) < static_cast<size_t>(Fog::Type::sCountOfNonDebugTypes)); 

    m_enabledStatus[static_cast<size_t>(type)] = true; 
}

bool FogEnabler::isEnabled(Fog::Type type) const
{
    assert(static_cast<size_t>(type) < static_cast<size_t>(Fog::Type::sCountOfNonDebugTypes));
    
    return m_enabledStatus[static_cast<size_t>(type)]; 
}
} // namespace service::simulation_controller