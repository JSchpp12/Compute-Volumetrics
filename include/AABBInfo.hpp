#pragma once

#include <glm/glm.hpp>

#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarBuffers/Buffer.hpp"
#include "TransferRequest_Buffer.hpp"

class AABBTransfer : public star::TransferRequest::Buffer
{
  public:
    AABBTransfer(const std::array<glm::vec4, 2> &aabbBounds, const uint32_t &computeQueueFamilyIndex,
                 const vk::DeviceSize &minUniformBufferOffsetAlignment)
        : aabbBounds(aabbBounds), computeQueueFamilyIndex(computeQueueFamilyIndex),
          minUniformBufferOffsetAlignment(minUniformBufferOffsetAlignment)
    {
    }

    std::unique_ptr<star::StarBuffers::Buffer> createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const override;

    std::unique_ptr<star::StarBuffers::Buffer> createFinal(vk::Device &device, VmaAllocator &allocator,
                                                  const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const override;

  protected:
    const std::array<glm::vec4, 2> aabbBounds;
    const uint32_t computeQueueFamilyIndex;
    const vk::DeviceSize minUniformBufferOffsetAlignment;
};

class AABBController : public star::ManagerController::RenderResource::Buffer
{
  public:
    AABBController(const std::array<glm::vec4, 2> &aabbBounds) : aabbBounds(aabbBounds)
    {
    }

    std::unique_ptr<star::TransferRequest::Buffer> createTransferRequest(star::core::device::StarDevice &device) override;

  private:
    const std::array<glm::vec4, 2> aabbBounds;
};