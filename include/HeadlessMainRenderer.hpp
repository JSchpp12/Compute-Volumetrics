#pragma once

#include <starlight/core/device/managers/ManagerCommandBuffer.hpp>
#include <starlight/core/renderer/DefaultRenderer.hpp>

class HeadlessMainRenderer : public star::core::renderer::DefaultRenderer
{
  public:
    virtual star::core::device::manager::ManagerCommandBuffer::Request getCommandBufferRequest() override;

  private:
};