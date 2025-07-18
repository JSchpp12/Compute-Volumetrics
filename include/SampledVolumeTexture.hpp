#pragma once

#include <memory>
#include <vector>

#include "ManagerController_RenderResource_Texture.hpp"
#include "StarTextures/Texture.hpp"
#include "TransferRequest_Texture.hpp"

class SampledVolumeRequest : public star::TransferRequest::Texture
{
  public:
    SampledVolumeRequest(const uint32_t &computeQueueFamilyIndex,
                         std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData)
        : sampledData(std::move(sampledData)), computeQueueFamilyIndex(computeQueueFamilyIndex)
    {
    }

    virtual std::unique_ptr<star::StarBuffer> createStagingBuffer(vk::Device &device,
                                                                  VmaAllocator &allocator) const override;

    virtual std::unique_ptr<star::StarTextures::Texture> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    virtual void copyFromTransferSRCToDST(star::StarBuffer &srcBuffer, star::StarTextures::Texture &dst,
                                          vk::CommandBuffer &commandBuffer) const override;

    virtual void writeDataToStageBuffer(star::StarBuffer &stagingBuffer) const override;

  private:
    const uint32_t computeQueueFamilyIndex;
    std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData;
};

class SampledVolumeController : public star::ManagerController::RenderResource::Texture
{
  public:
    SampledVolumeController(std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData)
        : sampledData(std::move(sampledData))
    {
    }

    std::unique_ptr<star::TransferRequest::Texture> createTransferRequest(star::StarDevice &device) override;

  protected:
    std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData;
};