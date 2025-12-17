#pragma once

#include <glm/glm.hpp>

#include <starlight/virtual/ManagerController_RenderResource_Buffer.hpp>
#include <starlight/wrappers/graphics/StarBuffers/Buffer.hpp>
#include <starlight/virtual/TransferRequest_Buffer.hpp>

class AABBTransfer : public star::TransferRequest::Buffer
{
  public:
    AABBTransfer(const uint32_t &computeQueueFamilyIndex, const std::array<glm::vec4, 2> &aabbBounds)
        : computeQueueFamilyIndex(computeQueueFamilyIndex), aabbBounds(aabbBounds)
    {
    }

    std::unique_ptr<star::StarBuffers::Buffer> createStagingBuffer(vk::Device &device,
                                                                   VmaAllocator &allocator) const override;

    std::unique_ptr<star::StarBuffers::Buffer> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const override;

  protected:
    const uint32_t computeQueueFamilyIndex;
    const std::array<glm::vec4, 2> aabbBounds;
};