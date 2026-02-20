#pragma once

#include <starlight/core/device/DeviceContext.hpp>

struct ISimulationController
{
    virtual ~ISimulationController() = default;
    virtual void init(star::core::device::DeviceContext &context) = 0;
    virtual void frameUpdate(star::core::device::DeviceContext &context) = 0; 
};