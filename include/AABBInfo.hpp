#pragma once

#include <array>
#include <glm/glm.hpp>

#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarBuffer.hpp"
#include "TransferRequest_Buffer.hpp"

class AABBTransfer : public star::TransferRequest::Buffer
{
  public:
    AABBTransfer(const std::array<glm::vec4, 2> &aabbBounds) : aabbBounds(aabbBounds)
    {
    }

    std::unique_ptr<star::StarBuffer> createStagingBuffer(vk::Device& device, VmaAllocator& allocator) const override; 

    std::unique_ptr<star::StarBuffer> createFinal(vk::Device &device, VmaAllocator &allocator) const override; 
        
    void writeDataToStageBuffer(star::StarBuffer& buffer) const override; 

  protected:
    const std::array<glm::vec4, 2> aabbBounds;
};

class AABBController : public star::ManagerController::RenderResource::Buffer
{
  public:
    AABBController(const std::array<glm::vec4, 2> &aabbBounds) : aabbBounds(aabbBounds)
    {
    }

    std::unique_ptr<star::TransferRequest::Buffer> createTransferRequest(
        const vk::PhysicalDevice &physicalDevice) override;

  private:
    const std::array<glm::vec4, 2> aabbBounds;
};