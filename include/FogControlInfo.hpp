#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "TransferRequest_Buffer.hpp"

class FogControlInfoTransfer : public star::TransferRequest::Buffer
{
  public:
    FogControlInfoTransfer(const float &fogNearDist, const float &fogFarDist);
    ~FogControlInfoTransfer() = default;

    std::unique_ptr<star::StarBuffer> createStagingBuffer(vk::Device& device, VmaAllocator& allocator, const uint32_t& transferQueueFamilyIndex) const override; 

    std::unique_ptr<star::StarBuffer> createFinal(vk::Device &device, VmaAllocator &allocator, const uint32_t& transferQueueFamilyIndex) const override; 

    void writeDataToStageBuffer(star::StarBuffer& buffer) const override; 

  private:
    float fogNearDist, fogFarDist;
};

class FogControlInfoController : public star::ManagerController::RenderResource::Buffer
{
  public:
    FogControlInfoController(const uint8_t &frameInFlightIndexToUpdateOn, const float &fogNearDist,
                             const float &fogFarDist);

    std::unique_ptr<star::TransferRequest::Buffer> createTransferRequest(
        star::StarDevice &device) override;

    bool isValid(const uint8_t &currentFrameInFlightIndex) const override;

  private:
    const float &currentFogNearDist, currentFogFarDist;
    float lastFogNearDist, lastFogFarDist;
};