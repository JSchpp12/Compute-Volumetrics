#include "HeadlessMainRenderer.hpp"

star::core::device::manager::ManagerCommandBuffer::Request HeadlessMainRenderer::getCommandBufferRequest()
{
    return star::core::device::manager::ManagerCommandBuffer::Request();
}
