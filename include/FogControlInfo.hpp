#pragma once

#include "FogInfo.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"
#include "TransferRequest_Buffer.hpp"

class FogControlInfoTransfer : public star::TransferRequest::Buffer
{
  public:
    FogControlInfoTransfer(const FogInfo::FinalizedInfo &fogInfo, const uint32_t &computeQueueFamilyIndex,
                           const vk::DeviceSize &minUniformBufferOffsetAlignment);
    ~FogControlInfoTransfer() = default;

    std::unique_ptr<star::StarBuffer> createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const override;

    std::unique_ptr<star::StarBuffer> createFinal(vk::Device &device, VmaAllocator &allocator,
                                                  const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(star::StarBuffer &buffer) const override;

  private:
    const uint32_t computeQueueFamilyIndex;
    const vk::DeviceSize minUniformBufferOffsetAlignment;
    const FogInfo::FinalizedInfo fogInfo;
};

class FogControlInfoController : public star::ManagerController::RenderResource::Buffer
{
  public:
    FogControlInfoController(const uint8_t &frameInFlightIndexToUpdateOn,
                             const std::shared_ptr<FogInfo> currentFogInfo);

    std::unique_ptr<star::TransferRequest::Buffer> createTransferRequest(star::StarDevice &device) override;

    bool isValid(const uint8_t &currentFrameInFlightIndex) const override;

  private:
    const std::shared_ptr<FogInfo> currentFogInfo = std::shared_ptr<FogInfo>();
    FogInfo lastFogInfo = FogInfo();
};