#pragma once

#include <star_common/Handle.hpp>

namespace renderer::finalization
{
class FinalizationRenderer
{
  public:
    virtual const star::Handle &getTimelineSemaphore(size_t index) const = 0;
    virtual const star::Handle &getCommandBuffer() const = 0; 
};
} // namespace renderer::finalization