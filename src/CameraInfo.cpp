#include "CameraInfo.hpp"

std::unique_ptr<star::StarBuffer> CameraInfo::createStagingBuffer(vk::Device &device, VmaAllocator &allocator, const uint32_t& transferQueueFamilyIndex) const{
    auto create = star::StarBuffer::BufferCreationArgs{sizeof(CameraData),
                                                    1,
                                                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                                    VMA_ALLOCATION_CREATE_MAPPED_BIT,
                                                    VMA_MEMORY_USAGE_AUTO,
                                                    vk::BufferUsageFlagBits::eTransferSrc,
                                                    vk::SharingMode::eConcurrent,
                                                    "CameraInfoBuffer_SRC"};

    return std::make_unique<star::StarBuffer>(allocator, create); 
}

std::unique_ptr<star::StarBuffer> CameraInfo::createFinal(vk::Device &device, VmaAllocator &allocator, const uint32_t& transferQueueFamilyIndex) const{
    auto create = star::StarBuffer::BufferCreationArgs{sizeof(CameraData),
                                                    1,
                                                    VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                                                    VMA_MEMORY_USAGE_AUTO,
                                                    vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
                                                    vk::SharingMode::eConcurrent,
                                                    "CameraInfoBuffer"};
    return std::make_unique<star::StarBuffer>(allocator, create); 
}

void CameraInfo::writeDataToStageBuffer(star::StarBuffer &buffer) const
{
    buffer.map();

    auto data = CameraData{glm::inverse(camera.getProjectionMatrix()),
                           glm::vec2(camera.getResolution()),
                           camera.getResolution().x / camera.getResolution().y,
                           camera.getFarClippingDistance(),
                           camera.getNearClippingDistance(),
                           tan(camera.getVerticalFieldOfView(true))};

    buffer.writeToBuffer(&data, sizeof(CameraData));

    buffer.unmap();
}

std::unique_ptr<star::TransferRequest::Buffer> CameraInfoController::createTransferRequest(
    star::StarDevice &device)
{
    return std::make_unique<CameraInfo>(this->camera);
}