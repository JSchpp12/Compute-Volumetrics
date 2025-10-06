#include "RandomValueTextureController.hpp"

#include "RandomValueTexture.hpp"

RandomValueTextureController::RandomValueTextureController(uint32_t width, uint32_t height)
    : m_width(std::move(width)), m_height(std::move(height))
{
}

std::unique_ptr<star::TransferRequest::Texture> RandomValueTextureController::createTransferRequest(
    star::core::device::StarDevice &device)
{
    return std::make_unique<RandomValueTexture>(
        m_width, m_height, device.getDefaultQueue(star::Queue_Type::Tcompute).getParentQueueFamilyIndex(),
        device.getPhysicalDevice().getProperties());
}
