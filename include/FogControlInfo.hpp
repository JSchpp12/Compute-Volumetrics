#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "TransferRequest_Buffer.hpp"

class FogControlInfoTransfer : public star::TransferRequest::Buffer
{
  public:
    FogControlInfoTransfer(const float &linearFog_nearDist, const float &linearFog_farDist, const float &expFog_density, const uint32_t &computeQueueFamilyIndex,
                           const vk::DeviceSize &minUniformBufferOffsetAlignment);
    ~FogControlInfoTransfer() = default;

    std::unique_ptr<star::StarBuffer> createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const override;

    std::unique_ptr<star::StarBuffer> createFinal(vk::Device &device, VmaAllocator &allocator,
                                                  const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(star::StarBuffer &buffer) const override;

  private:
    const uint32_t computeQueueFamilyIndex;
    const vk::DeviceSize minUniformBufferOffsetAlignment;
    float linearFog_nearDist, linearFog_farDist, expFog_density; 
};

class FogControlInfoController : public star::ManagerController::RenderResource::Buffer
{
  public:
    FogControlInfoController(const uint8_t &frameInFlightIndexToUpdateOn, const float &fogNearDist,
                             const float &fogFarDist, const float &expFog_density);

    std::unique_ptr<star::TransferRequest::Buffer> createTransferRequest(star::StarDevice &device) override;

    bool isValid(const uint8_t &currentFrameInFlightIndex) const override;

  private:
    const float &currentFogNearDist, &currentFogFarDist, &currentExpFog_density;
    float lastFogNearDist, lastFogFarDist, lastExpFog_density;
};