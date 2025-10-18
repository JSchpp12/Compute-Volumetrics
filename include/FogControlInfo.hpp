#pragma once

#include "FogInfo.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"
#include "TransferRequest_Buffer.hpp"

class FogControlInfoTransfer : public star::TransferRequest::Buffer
{
  public:
    FogControlInfoTransfer(const FogInfo::FinalizedInfo &fogInfo, const uint32_t &computeQueueFamilyIndex)
        : fogInfo(fogInfo), computeQueueFamilyIndex(computeQueueFamilyIndex)
    {
    }
    ~FogControlInfoTransfer() = default;

    std::unique_ptr<star::StarBuffers::Buffer> createStagingBuffer(vk::Device &device,
                                                                   VmaAllocator &allocator) const override;

    std::unique_ptr<star::StarBuffers::Buffer> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const override;

  private:
    const FogInfo::FinalizedInfo fogInfo;
    const uint32_t computeQueueFamilyIndex;
};

class FogInfoController : public star::ManagerController::RenderResource::Buffer
{
  public:
    FogInfoController() = default;

    virtual ~FogInfoController() = default;

    FogInfoController(std::shared_ptr<FogInfo> currentFogInfo)
        : currentFogInfo(std::move(currentFogInfo))
    {
    }

    std::unique_ptr<star::TransferRequest::Buffer> createTransferRequest(star::core::device::StarDevice &device,
                                                                         const uint8_t &frameInFlightIndex) override;

    bool needsUpdated(const uint8_t &currentFrameInFlightIndex) const override;

  private:
    const std::shared_ptr<FogInfo> currentFogInfo = std::shared_ptr<FogInfo>();
    FogInfo lastFogInfo = FogInfo();
};